#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>

#define MAX_LINE 100
#define MAX_INFO 30
#define MAX_TEXT 1024
#define MAX_GROUP 10


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
int s, reply_server, clients_port;
user_info user;
pthread_t msg_thread;
pthread_mutex_t mutex_listen = PTHREAD_MUTEX_INITIALIZER;
char groups[MAX_GROUP][MAX_INFO];
int n_groups = 0;

void *messages_incoming();
void client_server();
void p2p();
void group();
void join_group();
void multicast_message();
void listen_group_msg();
void *connections_types();


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

    clients_port = atoi(argv[2]);

    // UDP connection to server
    // socket for UDP packages
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		perror("Erro na criação do socket");
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(clients_port);
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);

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

        printf("Message received from %s: %s\n", reply.userName , reply.message);

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
    socklen_t client2_len = sizeof(client2_addr);
    user_info c_info;
    msg_t message_t;

    // stops client from receiving messages
    pthread_cancel(msg_thread);

    printf("Destination UserName: ");
    fgets(user.userName_dest, sizeof(user.userName_dest), stdin);
    user.userName_dest[strlen(user.userName_dest)-1] = 0;
    

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
    client2_addr.sin_family = AF_INET;
    client2_addr.sin_port = htons(clients_port);
    client2_addr.sin_addr.s_addr = inet_addr(c_info.user_IP);

    // gets message
    char message[1024];
    printf("Message: ");
    fgets(message, sizeof(message), stdin);
    message[strlen(message)-1] = 0;

    strcpy(message_t.userName, user.userName);
    strcpy(message_t.message, message);

    // sends message to client2
    sendto(s, &message_t, sizeof(message_t), 0, (struct sockaddr *) &client2_addr, (socklen_t ) client2_len);
    
    printf("Message send to %s: %s;\n", message_t.userName, message_t.message);
}


void group() {
    char txt[5];
    int option = -1;

    pthread_cancel(msg_thread);

    while(option != 0) {
        printf("Select option:\n\t1: Create/Join group\n\t2: Send message to group\n\t3: Listen for group messages\n");
        fgets(txt, sizeof(txt), stdin);
        option = atoi(txt);

        if (option == 1) {
            join_group();

        } else if (option == 2) {
            multicast_message();

        } else if (option == 3) {
            listen_group_msg();
        }

    }

    pthread_create(&msg_thread, NULL, messages_incoming, NULL);
}


void join_group() {
    char txt[5];
    int option = -1;
    msg_t msg;

    printf("Chose group IP:\n");
    for (int i = 0; i <n_groups; i++) {
        printf("\t%d: %s\n", i, groups[i]);
    }
    printf("\t%d: New Group", n_groups);
    fgets(txt, sizeof(txt), stdin);
    option = atoi(txt);

    strcpy(msg.message, "new group");

    if (option == n_groups) {
        sendto(s, &msg, sizeof(msg), 0, (struct sockaddr*)&serv_addr, (socklen_t ) slen);
    }

}

void multicast_message() {
    struct sockaddr_in groupSock;
    struct in_addr localInterface;
    msg_t message_t;
    char multicast_IP[MAX_INFO];
    //int sd;

    //sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    // gets message info
    // printf("Destination Group IP: ");
    // fgets(multicast_IP, sizeof(multicast_IP), stdin);
    strcpy(multicast_IP, "226.1.1.1");

    printf("Message: ");
    fgets(message_t.message, sizeof(message_t.message), stdin);
    message_t.message[strlen(message_t.message)-1] = 0;


    // initialize group socket
    memset((char *) &groupSock, 0, sizeof(groupSock));
    groupSock.sin_family = AF_INET;
    groupSock.sin_addr.s_addr = inet_addr(multicast_IP);
    groupSock.sin_port = htons(clients_port);


    printf("Group address: %s == UDP port: %d\n", inet_ntoa(groupSock.sin_addr), ntohs(groupSock.sin_port));
    printf("local interface: %s\n", inet_ntoa(localInterface));
    
    // change ttl
    int multicastTTL = 255;
    if(setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF, (char *)&multicastTTL, sizeof(multicastTTL)) < 0) {
        perror("Setting local interface error");
        return;
    } else {
        printf("Setting the local interface...OK\n");
    }

    // set interfaceIP
    localInterface.s_addr = inet_addr(user.ip);
    if(setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF, (char *)&localInterface, sizeof(localInterface)) < 0) {
        perror("Setting local interface error");
        return;
    } else {
        printf("Setting the local interface...OK\n");
    }

    //sendto(s, &message_t, sizeof(message_t), 0, (struct sockaddr*)&groupSock, sizeof(groupSock));
    if(sendto(s, &message_t, sizeof(message_t), 0, (struct sockaddr*)&groupSock, sizeof(groupSock)) < 0) {
        perror("Sending datagram message error");
    } else {
        printf("Sending datagram message...OK\n");
    }
 
    printf("Message sent\n");

}


void listen_group_msg() {
    struct sockaddr_in localSock;
    socklen_t group_len = sizeof(localSock);
    struct ip_mreq group;
    msg_t message_t;
    //int sd;

    // sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    int reuse = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0) {
        perror("Setting SO_REUSEADDR error");
        return;
    } else {
        printf("Setting SO_REUSEADDR...OK.\n");
    }

    memset((char *) &localSock, 0, sizeof(localSock));
    localSock.sin_family = AF_INET;
    localSock.sin_port = htons(clients_port);
    localSock.sin_addr.s_addr = INADDR_ANY;// inet_addr("127.0.0.1");

    group.imr_multiaddr.s_addr = inet_addr("226.1.1.1");
    group.imr_interface.s_addr = inet_addr(user.ip);

    printf("port: %d\n", clients_port);
    printf("Group address: %s == UDP port: %d\n", inet_ntoa(localSock.sin_addr), ntohs(localSock.sin_port));
    printf("local multiaddr: %s\n", inet_ntoa(group.imr_multiaddr));
    printf("local interface: %s\n", inet_ntoa(group.imr_interface));

    if(bind(s, (struct sockaddr*)&localSock, sizeof(localSock))) {
        perror("Binding datagram socket error");
        // return;
    }

    if(setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)) < 0) {
        perror("Adding multicast group error");
        return;
    } else {
        printf("Adding multicast group...OK.\n");
    }

    if((reply_server = recvfrom(s, &message_t, sizeof(message_t), 0, (struct sockaddr *) &localSock, (socklen_t *)&group_len)) == -1) {
        perror("Error on recvfrom");
        return;
	}
    printf("Reading datagram message...OK.\n");
    printf("The message from multicast server is: \"%s\"\n", message_t.message);

    /*
    if(read(s, &message_t, sizeof(message_t)) < 0) {
        perror("Reading datagram message error");
        return;
    } else {
        printf("Reading datagram message...OK.\n");
        printf("The message from multicast server is: \"%s\"\n", message_t.message);
    }*/

}



