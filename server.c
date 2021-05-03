#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_LINE 100
#define MAX_INFO 30

#define PORT 9876       // server port

typedef struct user_info{
    char userID[MAX_INFO];
} user_info;

int main(int argc, char** argv) {
    struct sockaddr_in si_minha, si_outra;
    socklen_t slen = sizeof(si_outra);
	int s, recv_len;

    if (argc != 4) {
        printf("server <port clients> <port config> <log file>\n");
        exit(-1);
    }

    // initialize UDP connection
    // Cria um socket para recepção de pacotes UDP
	if (( s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		perror("Erro na criação do socket");
	}

    // Preenchimento da socket address structure
	si_minha.sin_family = AF_INET;
	si_minha.sin_port = htons(PORT);
	si_minha.sin_addr.s_addr = htonl(INADDR_ANY);

    // Associa o socket à informação de endereço
	if (bind(s,(struct sockaddr*)&si_minha, sizeof(si_minha)) == -1) {
		perror("Erro no bind");
	}


    // initialize TCP connection

    char line[MAX_LINE];

    while(1) {
        if((recv_len = recvfrom(s, line, MAX_LINE, 0, (struct sockaddr *) &si_outra, (socklen_t *)&slen)) == -1) {
	        perror("Erro no recvfrom");
	    }
        printf("Received: %s\n", line);
    }

    /*while(1) {
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

    }*/

    return 0;
}