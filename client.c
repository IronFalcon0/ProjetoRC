#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>

#define MAX_LINE 100
#define MAX_INFO 30
#define MAX_TEXT 1024


typedef struct user_info {
    char behaviour[MAX_INFO];
    char userName[MAX_INFO];
    char ip[MAX_INFO];
    char password[MAX_INFO];
    char client_server;
    char p2p;
    char group;
    char autorized;
    char IP_dest[MAX_INFO];
    char message[MAX_TEXT];

} user_info;


struct sockaddr_in serv_addr;
socklen_t slen = sizeof(serv_addr);
int s, reply_server;
user_info user;
pthread_t msg_thread;

void *messages_incoming();
void client_server();
void p2p();
void group();


void sigusr1(int sig_num) {
    pthread_exit(NULL);
}


void sigint (int sig_num) {
    //kill thread
    pthread_kill(msg_thread, SIGUSR1);
    printf("Thread killed\n");

    exit(0);
}


int main(int argc, char** argv) {

    if (argc != 3) {
        printf("client <server address> <port>\n");
        exit(-1);
    }

    // UDP connection to server
    // socket for UDP packages
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		perror("Erro na criação do socket");
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &(serv_addr.sin_addr));
    //serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); 

    // ask user info
    printf("Username: ");
    fgets(user.userName, MAX_INFO, stdin);
    user.userName[strlen(user.userName)-1] = '\0';
    printf("Password: ");
    fgets(user.password, MAX_INFO, stdin);
    user.password[strlen(user.password)-1] = '\0';

    user.client_server = 0;
    user.p2p = 0;
    user.group = 0;
    user.autorized = 0;


    if (strlen(user.userName) == 0 || strlen(user.password) == 0) {
        printf("Invalid user name or password.\n");
        exit(0);
    }

    printf("Information send to server\n");
    strcpy(user.behaviour, "autentication");
    sendto(s, &user, sizeof(user), 0, (struct sockaddr *) &serv_addr, (socklen_t ) slen);

    if((reply_server = recvfrom(s, &user, sizeof(user), 0, (struct sockaddr *) &serv_addr, (socklen_t *)&slen)) == -1) {
	        perror("Error on recvfrom");
	}
    printf("%d\n", user.client_server);
    if (user.autorized == 0) {
        printf("Login without success\n");
        exit(0);
    }

    printf("Login with success\n");

    signal(SIGINT, sigint);

    // creates thread to receive messages
    pthread_create(&msg_thread, NULL, messages_incoming, NULL);

    int type = 0;
    while(1) {
        printf("Types of connections allowed:\n\t1 - client-server: %d\n\t2 - P2P: %d\n\t3 - group connection: %d\n", user.client_server, user.p2p, user.group);
        scanf("%d", &type);

        while(type < 0 && type > 3) {
            scanf("%d", &type);
        }
        
        if (type == 1 && user.client_server == 1) {
            strcpy(user.behaviour, "send_message");
            client_server();
        } else if (type == 2 && user.p2p == 1) {
            p2p();
        } else if (type == 3 && user.group == 1) {
            group();
        } else {
            printf("Connection type not allowed\n");
        }
    }

    return 0;
}

void *messages_incoming() {
    signal(SIGUSR1, sigusr1);
    // change user struct, crete new
    while(1) {
        if((reply_server = recvfrom(s, &user, sizeof(user), 0, (struct sockaddr *) &serv_addr, (socklen_t *)&slen)) == -1) {
	        perror("Error on recvfrom");
	    }
        printf("Message received: %s %s len(%d)!\n", user.userName, user.message, strlen(user.message));
    }
}



void client_server() {

    printf("IP destination: ");
    scanf("%s", &user.IP_dest);
    printf("Message: ");
    scanf("%s", &user.message);


    printf("Sending to %s: %s\n", user.IP_dest, user.message);
    sendto(s, &user, sizeof(user), 0, (struct sockaddr *) &serv_addr, (socklen_t ) slen);

}


void p2p() {

}


void group() {

}