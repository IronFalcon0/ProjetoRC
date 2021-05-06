#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define MAX_LINE 100
#define MAX_INFO 30
#define MAX_USERS 50

typedef struct user_info{
    char userID[MAX_INFO];
    char ip[MAX_INFO];
    char password[MAX_INFO];
    bool client_server;
    bool p2p;
    bool grupo;

} user_info;

user_info users[MAX_USERS];
int users_ids[MAX_USERS];   // current users connected
int n_users = 0;
int count = 0;

user_info *get_info(char *str);
void load_info(char file_name[]);
void printa(user_info *arr);

int main(int argc, char** argv){

    if (argc != 4) {
        printf("server <port clients> <port config> <log file>\n");
        exit(-1);
    }
    load_info(argv[3]);
    printa(users);

    // initialize UDP connection

    // initialize TCP connection

    char line[MAX_LINE];

    while(1) {
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

    }

    return 0;
}

user_info *get_info(char *str){
    char *tok;

    // change to static memory, use users[] and n_users
    user_info *user = malloc(sizeof(user_info));
    int cont = 0;
    tok = strtok(str," ");
    if(tok == NULL) return NULL;
    strcpy(user->userID,tok);
    for(cont = 0;tok != NULL;){
        tok = strtok(NULL, " ");
        if(tok != NULL){
            if(cont == 0) strcpy(user->ip,tok);
            if(cont == 1) strcpy(user->password,tok);
            if(cont == 2 && strlen(tok) == 3){
                user->client_server = tok[0] - '0';
                user->p2p = tok[1] - '0';
                user->grupo = tok[2] - '0';
            }
            cont ++;
        }
    }
    if(cont != 3){
        printf("ERRO de formatacao\n");
        return NULL;
    }else{
        return user;
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
    user_info *temp;
    while(tok != NULL){
        if((temp = get_info(tok)) != NULL) users[count++] = *temp;
        tok = strtok(NULL, "\n");
    }
}

void printa(user_info *arr){
  for(int i =0; i<count; i++){
    printf("UserID = %s\nip = %s\npassword = %s\nclient-server = %d\np2p = %d\ngrupo = %d\n",
    arr[i].userID,arr[i].ip,arr[i].password,arr[i].client_server,arr[i].p2p,arr[i].grupo);
  }
}
