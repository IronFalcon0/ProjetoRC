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


typedef struct user_info{
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

user_info users[MAX_USERS];
int users_ids[MAX_USERS];   // current users connected
int n_users = 0;
int fd_tcp;
char signal_exit = 0;

struct sockaddr_in serv_clients_addr, serv_config_addr, clients_addr;
socklen_t clients_len = sizeof(clients_addr);
int s_clients, recv_len, config_size;
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
void load_info(char []);
void printa(user_info *);
int find_user(char [], char []);
void *config_users(void*);
void autentication();
void create_connection();
void client_server();
void p2p();
void group_conn();


int main(int argc, char** argv){

    signal(SIGINT, sigint);


    if (argc != 4) {
        printf("server <port clients> <port config> <log file>\n");
        exit(-1);
    }

    load_info(argv[3]);
    printa(users);

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
    if( listen(fd_tcp, 5) < 0)
		perror("Error on function listen");

    // create thread to handle with config users
    pthread_create(&config_thread, NULL, config_users, NULL);


    // initialize UDP connection and verification
    // socket to receive UDP packages from clients
	if ((s_clients = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		perror("Error creating socket");
	}

	serv_clients_addr.sin_family = AF_INET;
	serv_clients_addr.sin_port = htons(atoi(argv[1]));
    serv_clients_addr.sin_addr.s_addr = htonl(INADDR_ANY);      // INADDR_ANY --> 0.0.0.0

    // bind socket
	if (bind(s_clients, (struct sockaddr*)&serv_clients_addr, sizeof(serv_clients_addr)) == -1) {
		perror("Error on function bind");
	}

    
    // print server info
    char server_addr[30];
	inet_ntop(AF_INET, &(serv_clients_addr.sin_addr), server_addr, INET_ADDRSTRLEN);
    printf("Server address: %s\n UDP port: %d\n TCP port: %d\n", server_addr, ntohs(serv_clients_addr.sin_port), ntohs(serv_config_addr.sin_port));
    
    while(1) {
        if((recv_len = recvfrom(s_clients, &info, MAX_LINE, 0, (struct sockaddr *) &clients_addr, (socklen_t *)&clients_len)) == -1) {
            perror("Error on recvfrom");
        }

        printf("%s and = %d\n", info.behaviour, strcmp(info.behaviour, "send_message"));
        if (strcmp(info.behaviour, "autentication") == 0){
            autentication();
            
        } else if (strcmp(info.behaviour, "send_message") == 0){
            client_server();
        }

        
        

        

    }
    
    return 0;
}

void client_server() {

    sendto(s_clients, &info.message, sizeof(info.message), 0, (struct sockaddr *) &clients_addr, (socklen_t ) clients_len);
    printf("Message successufly send to client with IP: %s! %s\n", info.IP_dest, info.message);

    while(strcpy(info.message, "exit") != 0) {
        // receive message from client1
        if((recv_len = recvfrom(s_clients, &info.message, sizeof(int), 0, (struct sockaddr *) &clients_addr, (socklen_t *)&clients_len)) == -1) {
            perror("Error on recvfrom");
        }

        // sends message to client2
        sendto(s_clients, &info.message, sizeof(info.message), 0, (struct sockaddr *) &clients_addr, (socklen_t ) clients_len);
        printf("Message successufly send to client with IP: %s! %s\n", info.IP_dest, info.message);
    }
}

void p2p() {

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

        sendto(s_clients, &info, sizeof(info), 0, (struct sockaddr *) &clients_addr, (socklen_t ) clients_len);
        printf("User autenticated\n");
    } else {
        info.autorized = 0;

        sendto(s_clients, &info, sizeof(info), 0, (struct sockaddr *) &clients_addr, (socklen_t ) clients_len);
        printf("User not autenticated\n");
    }
}


int find_user(char userName[MAX_INFO], char password[MAX_INFO]) {
    for (int i = 0; i< n_users; i++) {
        if (strcmp(userName, users[i].userName) == 0 && strcmp(password, users[i].password) == 0) {
                return i;
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
    
    //int i = 0;
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

