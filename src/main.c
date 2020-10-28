#include <stdio.h>
#include <stdlib.h>
#include <termios.h>

#include "flags.h"

int main(int argc, char **argv) {
    app = (applicationLayer*) malloc(sizeof(applicationLayer));
    llink = (linkLayer*) malloc(sizeof(linkLayer));
    
    // Parse commnad line arguments
    parse_flags(argc, argv);
              
    // Ask app to establish connection
    int fd = llopen(llink->port, app->status);

    if (fd < 0) {
        perror("Failed to establish connection.\n");
        exit(1);
    }

    file_transmission();

    llclose(fd); // TODO: Check for errors

    return 0;
}