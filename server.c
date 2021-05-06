#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>


#define MAX_LINE 100
#define MAX_INFO 30
#define MAX_USERS 50

#define PORT 9000      // server port


user_info users[MAX_USERS];
int users_ids[MAX_USERS];   // current users connected
int n_users = 0;


typedef struct user_info{
    char userName[MAX_INFO];
    char ip[MAX_INFO];
    char password[MAX_INFO];
    char client_server;
    char p2p;
    char group;
    char autorized;

} user_info;


void get_info(char *);

void load_info(char []);

void printa(user_info *);

int find_user(char [], char []);


int main(int argc, char** argv){
    struct sockaddr_in serv_addr, clients_addr;
    socklen_t slen = sizeof(clients_addr);
	int s, recv_len;

    if (argc != 4) {
        printf("server <port clients> <port config> <log file>\n");
        exit(-1);
    }

    load_info(argv[3]);
    printa(users);

    // initialize UDP connection and verification
    // Cria um socket para recepção de pacotes UDP
	if (( s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		perror("Erro na criação do socket");
	}

    // Preenchimento da socket address structure
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[1]));
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);      // INADDR_ANY --> 0.0.0.0

    // Associa o socket à informação de endereço
	if (bind(s,(struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
		perror("Erro no bind");
	}

    // clients autentication
    user_info info;
    while(1) {
        if((recv_len = recvfrom(s, &info, sizeof(info), 0, (struct sockaddr *) &clients_addr, (socklen_t *)&slen)) == -1) {
	        perror("Erro no recvfrom");
	    }
        printf("New user received\n");

        // check info
        if (find_user(info.userName, info.password) == 1) {
            info.autorized = 1;

            sendto(s, &info, sizeof(info), 0, (struct sockaddr *) &clients_addr, (socklen_t ) slen);
            printf("User autenticated\n");
        } else {
            info.autorized = 0;

            sendto(s, &info, sizeof(info), 0, (struct sockaddr *) &clients_addr, (socklen_t ) slen);
            printf("User not autenticated\n");
        }


    }

    // print server info
    char server_addr[30];
	inet_ntop(AF_INET, &(serv_addr.sin_addr), server_addr, INET_ADDRSTRLEN);
    printf("server address: %s port: %d\n", server_addr, ntohs(serv_addr.sin_port));
    
    return 0;
}

int find_user(char userName[MAX_INFO], char password[MAX_INFO]) {
    for (int i = 0; i< n_users; i++) {
        if (strcmp(userName, users[i].userName) == 0 && strcmp(password, users[i].password) == 0) {
                return 1;
        }
    }
    return 0;
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
        printf("ERRO de formatacao\n");
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
        printf("ERRO AO ABRIR FICHEIRO\n");

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
}s
