#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_LINE 100

int main(int argc, char** argv) {

    if (argc != 4) {
        printf("server <port clients> <port config> <log file>\n");
        exit(-1);
    }

    // initialize UDP connection

    // initialize TCP connection

    char line[MAX_LINE];

    while(1) {
        fgets(line, 10, stdin);
        line[strlen(line)-1] = '\0';

        if (strcmp(line, "QUIT") == 0) {
            // close connection between client and server
			break;

		} else if (strcmp(line, "LIST") == 0) {
            // reply with information from log file
            continue;
        
        } else if (strcmp(line, "ADD") == 0) {
            // add user info to log file
            // add_user();
            continue;
        
        } else if (strcmp(line, "DEL") == 0) {
            // delete user info from log file
            // del_user();
            continue;
        }

    }

    return 0;
}