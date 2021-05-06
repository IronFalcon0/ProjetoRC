#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAX_LINE 100
#define MAX_INFO 30

#define SERVER_PORT 8000

typedef struct user_info_c{
    char userName[MAX_INFO];
    char password[MAX_INFO];
    int token;
    char autorized;
    
} user_info_c;


int main(int argc, char** argv) {
    struct sockaddr_in serv_addr;
    socklen_t slen = sizeof(serv_addr);
    int s, reply_server;

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
	serv_addr.sin_port = htons(SERVER_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    //inet_pton(AF_INET, argv[1], &(serv_addr.sin_addr));
    // works with INADDR_ANY but not with 206.254.113.35

    // ask user info
    user_info_c user;
    printf("Username: ");
    fgets(user.userName, MAX_INFO, stdin);
    printf("Password: ");
    fgets(user.password, MAX_INFO, stdin);

    user.token = 0;
    user.autorized = 0;

    if (strlen(user.userName) == 0 || strlen(user.password) == 0) {
        printf("Invalid user name or password.\n");
        exit(0);
    }

    printf("Information send to server\n");
    sendto(s, &user, sizeof(user), 0, (struct sockaddr *) &serv_addr, (socklen_t ) slen);

    if((reply_server = recvfrom(s, &user, sizeof(user), 0, (struct sockaddr *) &serv_addr, (socklen_t *)&slen)) == -1) {
	        perror("Erro no recvfrom");
	}
    printf("Respond from server: token:%d; autorized: %d", user.token, user.autorized);


    /*
    char server_addr[30];
	inet_ntop(AF_INET, &(serv_addr.sin_addr), server_addr, INET_ADDRSTRLEN);
    printf("server address: %s port: %d\n", server_addr, ntohs(serv_addr.sin_port));
    // test udp connection
    char line[MAX_LINE];
    char reply[MAX_LINE];

    //autenticate_user
    strcpy(line, "try to autenticate");
    sendto(s, line, strlen(line), 0, (struct sockaddr *) &serv_addr, (socklen_t ) slen);
    printf("Send to server: %s\n", line);

    if((reply_server = recvfrom(s, reply, MAX_LINE, 0, (struct sockaddr *) &serv_addr, (socklen_t *)&slen)) == -1) {
	        perror("Erro no recvfrom");
	}
    printf("token received: %s!\n", reply);*/

    /*
    while(1) {
        fgets(line, 10, stdin);
        line[strlen(line)-1] = '\0';
        sendto(s, line, strlen(line), 0, (struct sockaddr *) &serv_addr, (socklen_t ) slen);
        printf("Send to server: %s\n", line);
    }*/

    return 0;
}