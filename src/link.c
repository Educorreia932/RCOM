#include "link.h"

struct linkLayer llink;
struct termios oldtio, newtio;
struct sigaction action;
int fd;

bool flag = true;
int alarm_counter = 0;

void atende() { // atende alarme
	printf("alarme # %d\n", alarm_counter);
	flag = true;
	alarm_counter++;
}

int send_supervision_frame(int fd, char a, char c) {
	unsigned char buf[5];

	buf[0] = FLAG;
	buf[1] = a;
	buf[2] = c;
	buf[3] = a ^ c; // BCC
	buf[4] = FLAG;

	return write(fd, buf, 5);
}

int send_information_frame(int fd, char a, char c) {
	unsigned char buf[5];

	// TODO:
	
	return write(fd, buf, 5);
}

int establish_connection(char * port, enum Status status) {
	char buf[1];
	
    strcpy(llink.port, port);
	llink.baudRate = BAUDRATE;
	llink.sequenceNumber = 0;
	llink.timeout = TIMEOUT;
	llink.numTransmissions = NUM_TRANSMITIONS;

    /*
	Open serial port device for reading and writing and not as controlling tty
	because we don't want to get killed if linenoise sends CTRL-C.
	*/
    fd = open(port, O_RDWR | O_NOCTTY);

	if (fd < 0) {
		perror(port);
		exit(-1);
	}

	/* Save current port settings */
	if (tcgetattr(fd, &oldtio) == -1) { 
		perror("tcgetattr");
		exit(-1);
	}

	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;

	/* Set input mode (non-canonical, no echo,...) */
	newtio.c_lflag = 0;

	newtio.c_cc[VTIME] = 0; /* Inter-character timer unused */
	newtio.c_cc[VMIN] = 1;  /* Blocking read until 5 chars received */

	/*
	VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
	leitura do(s) próximo(s) caracter(es)
	*/

	tcflush(fd, TCIOFLUSH);

	if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
		perror("tcsetattr");
		exit(-1);
	}

	printf("New termios structure set\n");

	// Set state machine
	struct state_machine stm;

	stm.status = status;
	stm.current_state = START;

	if (status == TRANSMITTER) {
		// Set alarm handler
		action.sa_handler = &atende;
		sigemptyset (&action.sa_mask);
		action.sa_flags = 0;

		// Installs co-routine that attends interruption
		if (sigaction(SIGALRM, &action, NULL) < 0) {
			perror("Failed to set SIGALARM handler.\n");
			exit(1);
		}

		bool receivedUA = false;

		// Tries to send Set Message
		while (alarm_counter < llink.numTransmissions) {
			if (flag) {
				alarm(llink.timeout);  // Activactes 3 second alarm
				flag = false;

				int n = send_supervision_frame(fd, A_EM_CMD, C_SET); //sends SET message

				if (n == -1) {
					perror("Failed to send SET message.");
					exit(1);
				}

				printf("Sent SET message. \n");
			}

			// Tries to receive UA message
			if (read(fd, buf, 1) < 0) {
				// Read was not interrupted by the alarm, so it wasn't the cause for errno
				if (errno != EINTR) {
					perror("Failed to read.");
					exit(1);		
				}
			}

			// No error occured
			else {
				printf("Não devia %x\n", buf[0]);
				change_state(stm, buf[0]);  // Check if it is UA msg (state machine)

				if (stm.current_state == STOP)
					receivedUA = true;
			}
			
			if (receivedUA) {
				printf("Received UA message.\n");
				break; // Read successfully UA msg
			}
		}
	}

	else if (status == RECEIVER) {
		bool receivedSet = false;

		while(!receivedSet)
		{
			// Tries to receive SET message
			if (read(fd, buf, 1) < 0) {
				perror("Failed to read.");
				exit(1);		
			}

			else {
				change_state(stm, buf[0]);  // Check if it is SET msg (state machine)
				printf("%u\n", stm.current_state);
				if (stm.current_state == STOP)
					receivedSet = true;
			}
		}
		
		int n = send_supervision_frame(fd, A_RC_RESP, C_UA); // Sends UA message

		if (n == -1) {
			perror("Failed to send UA message.");
			exit(1);
		}

		printf("Sent UA message.\n");
	}
}