#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAX_LINE 100

#define PORT 9876

int main(int argc, char** argv) {
    struct sockaddr_in serv_addr;
    socklen_t slen = sizeof(serv_addr);
    int s;

    if (argc != 3) {
        printf("client <server address> <port>\n");
        exit(-1);
    }

    // UDP connection to server
    // Cria um socket para recepção de pacotes UDP
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		perror("Erro na criação do socket");
	}

    // Preenchimento da socket address structure
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // test udp connection
    char line[MAX_LINE];

    while(1) {
        fgets(line, 10, stdin);
        line[strlen(line)-1] = '\0';
        sendto(s, line, strlen(line), 0, (struct sockaddr *) &serv_addr, (socklen_t ) slen);
        printf("Send to server: %s\n", line);
    }

    return 0;
}