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

int establish_connection(char * port, enum Status stat) {
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


	//Set alarm handler
	if(stat == TRANSMITTER) {
		//Alarm
		action.sa_handler = &atende;
		sigemptyset (&action.sa_mask);
		action.sa_flags = 0;

		if( sigaction(SIGALRM, &action, NULL) < 0)  // Installs co-routine that attends interruption
		{
			perror("Failed to set SIGALARM handler.\n");
			exit(1);
		}
	}

	//Set state machine
	struct state_machine stm;
	
	stm.status = stat;
	stm.current_state = START;

	//Send SET message if transmitter 
	//Receive SET message if receptor

	
	//Send UA message if transmitter
	//Receive UA message if receptor

    return fd;
}

