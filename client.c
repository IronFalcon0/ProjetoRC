#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>

#define MAX_LINE 100
#define MAX_INFO 30
#define MAX_TEXT 1024


typedef struct user_info {
    char behavior[MAX_INFO];
    char userName[MAX_INFO];
    char ip[MAX_INFO];
    char password[MAX_INFO];
    char client_server;
    char p2p;
    char group;
    char autorized;
    char userName_dest[MAX_INFO];
    char message[MAX_TEXT];
    char user_IP[MAX_INFO];

} user_info;

typedef struct msg_t {
    char userName[MAX_INFO];
    char message[MAX_TEXT];
} msg_t;


struct sockaddr_in serv_addr;
socklen_t slen = sizeof(serv_addr);
int s, reply_server;
user_info user;
pthread_t msg_thread;
pthread_mutex_t mutex_listen = PTHREAD_MUTEX_INITIALIZER;

void *messages_incoming();
void client_server();
void p2p();
void group();


void sigint (int sig_num) {
    //kill thread
    pthread_cancel(msg_thread);
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

    printf("Client address: %s == UDP port: %d\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));

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
    strcpy(user.behavior, "autentication");
    sendto(s, &user, sizeof(user), 0, (struct sockaddr *) &serv_addr, (socklen_t ) slen);

    if((reply_server = recvfrom(s, &user, sizeof(user), 0, (struct sockaddr *) &serv_addr, (socklen_t *)&slen)) == -1) {
	        perror("Error on recvfrom");
	}
    
    if (user.autorized == 0) {
        printf("Login without success\n");
        exit(0);
    }


    printf("Login with success\n");

    signal(SIGINT, sigint);

    // creates thread to receive messages
    pthread_create(&msg_thread, NULL, messages_incoming, NULL);

    char s_type[3];
    int type = 0;
    while(1) {
        printf("\nTypes of connections allowed:\n\t1 - client-server: %d\n\t2 - P2P: %d\n\t3 - group connection: %d\n", user.client_server, user.p2p, user.group);
        fgets(s_type, sizeof(s_type), stdin);
        type = atoi(s_type);
        
        if (type == 1 && user.client_server == 1) {
            strcpy(user.behavior, "client_server");
            client_server();

        } else if (type == 2 && user.p2p == 1) {
            strcpy(user.behavior, "p2p");
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
    msg_t reply;

    while(1) {
        if((reply_server = recvfrom(s, &reply, sizeof(reply), 0, (struct sockaddr *) &serv_addr, (socklen_t *)&slen)) == -1) {
	        perror("Error on recvfrom");
	    }

        printf("Message received from %s: %s\n", reply.userName , reply.message);  // %s len(%ld)!    reply.userName, , strlen(reply.message)

    }
}



void client_server() {

    printf("Destination UserName: ");
    fgets(user.userName_dest, sizeof(user.userName_dest), stdin);
    user.userName_dest[strlen(user.userName_dest)-1] = 0;

    printf("Message: ");
    fgets(user.message, sizeof(user.message), stdin);
    user.message[strlen(user.message)-1] = 0;

    sendto(s, &user, sizeof(user), 0, (struct sockaddr *) &serv_addr, (socklen_t ) slen);

}


void p2p() {
    struct sockaddr_in client2_addr;
    socklen_t client_len = sizeof(client2_addr);
    user_info c_info;
    msg_t message_t;

    // stops client from receiving messages
    pthread_cancel(msg_thread);

    printf("Destination UserName: ");
    fgets(user.userName_dest, sizeof(user.userName_dest), stdin);
    user.userName_dest[strlen(user.userName_dest)-1] = 0;
    
    // mutex lock to block thread from receiving message

    // sends UserName of destination
    printf("Sending to %s\n", user.userName_dest);
    sendto(s, &user, sizeof(user), 0, (struct sockaddr *) &serv_addr, (socklen_t ) slen);

    // receives from server IP of destination
    if((reply_server = recvfrom(s, &c_info, sizeof(c_info), 0, (struct sockaddr *) &serv_addr, (socklen_t *)&slen)) == -1) {
	        perror("Error on recvfrom");
	}

    // alows client to receive messages
    pthread_create(&msg_thread, NULL, messages_incoming, NULL);


    printf("IP dest: %s\n", c_info.user_IP);

    if (strcmp(c_info.user_IP, "not found") == 0){
        printf("User IP not found\n");
        return;
    }

    // changes IP to client2
    char IP[100];
    strcpy(IP, c_info.user_IP);
    printf("%s\n", IP);
    client2_addr.sin_family = AF_INET;
    client2_addr.sin_port = serv_addr.sin_port;
    inet_pton(AF_INET, IP ,&(client2_addr.sin_addr));

    // gets message
    char message[1024];
    printf("Message: ");
    fgets(message, sizeof(message), stdin);
    message[strlen(message)-1] = 0;

    strcpy(message_t.userName, user.userName);
    strcpy(message_t.message, message);

    printf("Mess== %s !!! %s ;\n", message_t.message, message_t.userName);

    // sends message to client2
    sendto(s, &message_t, sizeof(message_t), 0, (struct sockaddr *) &client2_addr, (socklen_t ) client_len);
    

}


void group() {

    pthread_cancel(msg_thread);



    pthread_create(&msg_thread, NULL, messages_incoming, NULL);

}