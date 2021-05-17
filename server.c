#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
// #include <sys/types.h>
// #include <arpa/inet.h>
// #include <netdb.h>
// #include <netinet/in.h>
#include <sys/wait.h>


#define MAX_LINE 100
#define MAX_INFO 30
#define MAX_USERS 50
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
    char userName_dest[MAX_INFO];
    char message[MAX_TEXT];
    char user_IP[MAX_INFO];
    int port;

} user_info;


user_info users[MAX_USERS];
int n_users = 0;
int fd_tcp;

struct sockaddr_in serv_addr, serv_config_addr, clients_addr;
socklen_t clients_len = sizeof(clients_addr);
socklen_t config_size = sizeof(serv_config_addr);
int s_clients, recv_len;
user_info info;

pthread_t config_thread;


void sigusr1(int sig_num) {
    pthread_exit(NULL);
}


void sigint (int sig_num) {
    //close TCP connection
    close(fd_tcp);

    //kill thread
    pthread_kill(config_thread, SIGUSR1);
    printf("Thread killed\n");

    exit(0);
}


void get_info(char *);
char *find_user_ip(char []); 
void load_info(char []);
void printa(user_info *);
int find_user(char [], char []);
void *config_users(void*);
void autentication();
void create_connection();
void client_server();
void p2p(int);
void group_conn();


int main(int argc, char** argv){

    signal(SIGINT, sigint);

    if (argc != 4) {
        printf("server <port clients> <port config> <log file>\n");
        exit(-1);
    }

    // load user info from log file
    load_info(argv[3]);

    // initialize TCP connection
    bzero((void *) &serv_config_addr, sizeof(serv_config_addr));
    serv_config_addr.sin_family = AF_INET;
    serv_config_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_config_addr.sin_port = htons(atoi(argv[2]));

    if ((fd_tcp = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		perror("Error on function socket");

    if (bind(fd_tcp, (struct sockaddr*) &serv_config_addr,sizeof(serv_config_addr)) < 0)
		perror("Error on function bind");

    // wait for connections
    if(listen(fd_tcp, 5) < 0)
		perror("Error on function listen");

    // create thread to handle config users
    pthread_create(&config_thread, NULL, config_users, NULL);


    // initialize UDP connection
	if ((s_clients = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		perror("Error creating socket");
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[1]));
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);      // INADDR_ANY --> 0.0.0.0
    // clients_addr.sin_port = htons(atoi(argv[1]));  // don't know if needed?!

	if (bind(s_clients, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
		perror("Error on function bind");
	}

    
    // print server info
    char server_addr[30];
	inet_ntop(AF_INET, &(serv_addr.sin_addr), server_addr, INET_ADDRSTRLEN);
    printf("Server address: %s == UDP port: %d == TCP port: %d == Clients_port: %d\n", server_addr, ntohs(serv_addr.sin_port), ntohs(serv_config_addr.sin_port), ntohs(clients_addr.sin_port));
    
    // receive info from clients and responds accordingly
    while(1) {
        if((recv_len = recvfrom(s_clients, &info, sizeof(info), 0, (struct sockaddr *) &clients_addr, (socklen_t *)&clients_len)) == -1) {
            perror("Error on recvfrom");
        }

        printf("IP: %s === Port: %d\n", inet_ntoa(clients_addr.sin_addr), ntohs(clients_addr.sin_port));

        if (strcmp(info.behaviour, "autentication") == 0){
            autentication();

        } else if (strcmp(info.behaviour, "client_server") == 0){
            client_server();
        } else if (strcmp(info.behaviour, "p2p") == 0){
            p2p(*argv[1]);
        }
    }
    
    return 0;
}


void client_server() {
    char oldIP[MAX_INFO];
    char *IP_dest;
    user_info reply;

    // get client2 ip
    IP_dest = find_user_ip(info.userName_dest);
    printf("IP_dest: %s\n", IP_dest);

    if (strcmp(IP_dest, "not found") == 0) {
        printf("UserName not found, message not delivered: %s\n", info.message);

        strcpy(reply.message, "Message not send");
        sendto(s_clients, &reply, sizeof(reply), 0, (struct sockaddr *) &clients_addr, (socklen_t ) clients_len);
        return;
    }

    inet_ntop(AF_INET, &(serv_addr.sin_addr), oldIP, INET_ADDRSTRLEN);

    printf("oldIP: %s\n", oldIP);
    printf("===%d=\n", inet_pton(AF_INET, IP_dest, &(serv_addr.sin_addr)));

    // change ip to client2
    serv_addr.sin_addr.s_addr = htonl(inet_pton(AF_INET, IP_dest, &(serv_addr.sin_addr)));

    if (bind(s_clients, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
		perror("Error on function bind");
	}

    sendto(s_clients, &info, sizeof(info), 0, (struct sockaddr *) &clients_addr, (socklen_t ) clients_len);
    printf("Message successufly send to client with IP: %s! %s\n", IP_dest, info.message);

    // sets ip back to old
    serv_addr.sin_addr.s_addr = htonl(inet_pton(AF_INET, oldIP, &(serv_addr.sin_addr)));

    if (bind(s_clients, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
		perror("Error on function bind");
	}

    // send confirmation of success
    strcpy(reply.message, "Message send successufly");
    sendto(s_clients, &reply, sizeof(reply), 0, (struct sockaddr *) &clients_addr, (socklen_t ) clients_len);

}


void p2p( int c_port) {
    char *IP_dest;
    user_info dest_info;

    // get client2 ip
    IP_dest = find_user_ip(info.userName_dest);
    printf("IP_dest: %s\n", IP_dest);

    if (strcmp(IP_dest, "not found") == 0) {
        printf("UserName not found, message not delivered: %s\n", info.message);
        return;
    }
    // gets client info
    strcpy(dest_info.user_IP, IP_dest);
    dest_info.port = c_port;

    sendto(s_clients, &dest_info, sizeof(dest_info), 0, (struct sockaddr *) &clients_addr, (socklen_t ) clients_len);

}


void group_conn() {

}


void autentication() {
    printf("New user received\n");

    // check info
    int user_index = find_user(info.userName, info.password);
    if (user_index != -1) {
        info.autorized = 1;
        info.client_server = users[user_index].client_server;
        info.p2p =  users[user_index].p2p;
        info.group =  users[user_index].group;

        printf("User autenticated\n");

    } else {
        info.autorized = 0;
        printf("User not autenticated\n");
    }
    sendto(s_clients, &info, sizeof(info), 0, (struct sockaddr *) &clients_addr, (socklen_t ) clients_len);
}


int find_user(char userName[MAX_INFO], char password[MAX_INFO]) {
    for (int i = 0; i< n_users; i++) {
        if (strcmp(userName, users[i].userName) == 0 && strcmp(password, users[i].password) == 0) {
                return i;
        }
    }
    return -1;
}


char *find_user_ip(char userName[MAX_INFO]) {
    for (int i = 0; i < n_users; i++) {
        if (strcmp(userName, users[i].userName) == 0) {
            return users[i].ip;
        }
    }
    return "not found";

}


int find_user_port(char userName[MAX_INFO]) {
    for (int i = 0; i < n_users; i++) {
        if (strcmp(userName, users[i].userName) == 0) {
            //return users[i].port;
            return 1;
        }
    }
    return -1;

}

void get_info(char *str){
    char *tok;
    user_info user;
    int count = 0;
    tok = strtok(str," ");
    strcpy(user.userName,tok);
    
    for(count = 0; tok != NULL;){
        tok = strtok(NULL, " ");

        if(tok != NULL){
            if(count == 0) strcpy(user.ip,tok);
            if(count == 1) strcpy(user.password,tok);
            if(count == 2){
                user.client_server = tok[0] - '0';
                user.p2p = tok[1] - '0';
                user.group = tok[2] - '0';
            }
            count ++;
        }
    }

    if(count != 3){
        printf("Wrong format\n");
    }else{
        users[n_users] = user;
        n_users++;
    }

}


void load_info(char file_name[]){
    char line[MAX_LINE];
    FILE *fp;
    
    fp = fopen(file_name, "r");

    if (fp == NULL)// erro a abrir ficheiro
        printf("Error opening file %s\n", file_name);

    while(fgets(line, MAX_LINE, fp) != NULL) {
        char *tok = strtok(line,"\n");
        
        while(tok != NULL){
            get_info(tok);
            tok = strtok(NULL, "\n");
        }
    }
}


void printa(user_info *arr){
    for(int i =0; i<n_users; i++){
        printf("User %d:\n\tuserName = %s\n\tip = %s\n\tpassword = %s\n\tclient-server = %d\n\tp2p = %d\n\tgrupo = %d\n\n",
        i, arr[i].userName,arr[i].ip,arr[i].password,arr[i].client_server,arr[i].p2p,arr[i].group);
    }
}


void *config_users(void* i) {
    signal(SIGUSR1, sigusr1);
    printf("Thread initialized\n");
    
    int conf = 0, nread = 0;
    char line[MAX_LINE];
    while(1) {
        while(waitpid(-1,NULL,WNOHANG)>0);

        conf = accept(fd_tcp,(struct sockaddr *) &serv_config_addr,(socklen_t *)&config_size);
        
        if (conf > 0) {
            nread = read(conf, line, MAX_LINE-1);
            if (nread != 0) {
                printf("Received config: %s", line);
                // commands TODO

            }

        }

    }
}

