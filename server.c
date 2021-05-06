#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define MAX_LINE 100
#define MAX_INFO 30
#define MAX_USERS 50

#define PORT 9876       // server port

typedef struct user_info{
    char userID[MAX_INFO];
    char ip[MAX_INFO];
    char password[MAX_INFO];
    char client_server;
    char p2p;
    char grupo;

} user_info;

typedef struct user_info_c{
    char userName[MAX_INFO];
    char password[MAX_INFO];
    int token;
    char autorized;
    
} user_info_c;

user_info users[MAX_USERS];
int users_ids[MAX_USERS];   // current users connected
int n_users = 0;



int main(int argc, char** argv){
    struct sockaddr_in serv_addr, clients_addr;
    socklen_t slen = sizeof(clients_addr);
	int s, recv_len;

    if (argc != 4) {
        printf("server <port clients> <port config> <log file>\n");
        exit(-1);
    }

    void get_info(char *str);
    void load_info(char file_name[]);
    void printa(user_info *arr);

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
    user_info_c info;
    while(1) {
        if((recv_len = recvfrom(s, &info, sizeof(info), 0, (struct sockaddr *) &clients_addr, (socklen_t *)&slen)) == -1) {
	        perror("Erro no recvfrom");
	    }
        printf("New user received\n");

        // check info

        info.autorized = 1;
        info.token = 1;

        sendto(s, &info, sizeof(info), 0, (struct sockaddr *) &clients_addr, (socklen_t ) slen);
        printf("User autenticated\n");


    }

    // print server info
    char server_addr[30];
	inet_ntop(AF_INET, &(serv_addr.sin_addr), server_addr, INET_ADDRSTRLEN);
    printf("server address: %s port: %d\n", server_addr, ntohs(serv_addr.sin_port));
    /*
    // verification from client
    char line[MAX_LINE];
    char reply[MAX_LINE];

    if((recv_len = recvfrom(s, line, MAX_LINE, 0, (struct sockaddr *) &clients_addr, (socklen_t *)&slen)) == -1) {
	        perror("Erro no recvfrom");
	}
    printf("%s\n", line);

    // print client info
    char client_addr[MAX_LINE];
    inet_ntop(AF_INET, &(clients_addr.sin_addr), client_addr, INET_ADDRSTRLEN);
    printf("server address: %s port: %d\n", server_addr, ntohs(clients_addr.sin_port));

    // reply = autenticate_user(line);
    strcpy(reply, "autentication done");
    sendto(s, reply, strlen(reply), 0, (struct sockaddr *) &clients_addr, (socklen_t ) slen);
    printf("%s\n", reply);*/
    
    // initialize TCP connection

    
    /*
    while(1) {
        if((recv_len = recvfrom(s, line, strlen(line), 0, (struct sockaddr *) &clients_addr, (socklen_t *)&slen)) == -1) {
	        perror("Erro no recvfrom");
	    }
        printf("Received: %s\n", line);
    }*/

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

void get_info(char *str){
    char *tok;
    user_info user;
    int cont = 0;
    tok = strtok(str," ");
    strcpy(user.userID,tok);
    for(cont = 0;tok != NULL;){
        tok = strtok(NULL, " ");
        if(tok != NULL){
            if(cont == 0) strcpy(user.ip,tok);
            if(cont == 1) strcpy(user.password,tok);
            if(cont == 2 && strlen(tok) == 3){
                user.client_server = tok[0] - '0';
                user.p2p = tok[1] - '0';
                user.grupo = tok[2] - '0';
            }
            cont ++;
        }
    }

    if(cont != 3){
        printf("ERRO de formatacao\n");
    }else{
        users[n_users++] = user;
    }

}

void load_info(char file_name[]){
    char ch[1000];
    int aux;
    FILE *fp;
    int i = 0;
    fp = fopen(file_name, "r");

    if (fp == NULL)// erro a abrir ficheiro
        printf("ERRO AO ABRIR FICHEIRO\n");
    while((aux = fgetc(fp))!= EOF){
        ch[i++] = aux;
    }

    ch[i] =0;
    char *tok = strtok(ch,"\n");
    while(tok != NULL){
        get_info(tok);
        tok = strtok(NULL, "\n");
    }
}

void printa(user_info *arr){
    for(int i =0; i<n_users; i++){
        printf("UserID = %s\nip = %s\npassword = %s\nclient-server = %d\np2p = %d\ngrupo = %d\n",
        arr[i].userID,arr[i].ip,arr[i].password,arr[i].client_server,arr[i].p2p,arr[i].grupo);
    }
}
