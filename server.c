#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
// #include <sys/types.h>
// #include <arpa/inet.h>
// #include <netdb.h>
// #include <netinet/in.h>
#include <sys/wait.h>
#include <errno.h>


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

} user_info;

char file_name[MAX_LINE];
user_info users[MAX_USERS];
int n_users = 0;
int fd_tcp;

struct sockaddr_in serv_addr, client_addr, serv_config_addr;
socklen_t serv_len = sizeof(serv_addr);
socklen_t client_len = sizeof(client_addr);
socklen_t config_size = sizeof(serv_config_addr);
int s_clients, recv_len;
user_info info;

pthread_t config_thread;


void sigint (int sig_num) {
    //close TCP connection
    close(fd_tcp);

    //kill thread
    pthread_cancel(config_thread);
    printf("Thread killed\n");

    exit(0);
}


int get_info(char *);
char *find_user_ip(char []); 
void load_info();
void printa(user_info *);
int find_user(char [], char []);
void *config_users(void*);
void autentication();
void create_connection();
void client_server();
void p2p();
void group_conn();


int main(int argc, char** argv){

    sprintf(file_name,"%s",argv[3]);

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
    inet_aton("127.0.0.1", &serv_config_addr.sin_addr);
    //serv_config_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_config_addr.sin_port = htons(atoi(argv[2]));

    if ((fd_tcp = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Error on function socket");
        exit(0);
    }

    if (bind(fd_tcp, (struct sockaddr*) &serv_config_addr,sizeof(serv_config_addr)) < 0) {
		perror("Error on function bind");
        exit(0);
    }

    // wait for connections
    if(listen(fd_tcp, 5) < 0) {
		perror("Error on function listen");
        exit(0);
    }

    // create thread to handle config users
    pthread_create(&config_thread, NULL, config_users, NULL);

    // initialize UDP connection
	if ((s_clients = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		perror("Error creating socket");
        exit(0);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[1]));
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);      // INADDR_ANY --> 0.0.0.0
    

	if (bind(s_clients, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
		perror("Error on function bind");
        exit(0);
	}


    // print server info
    printf("Server address: %s == UDP port: %d == TCP port: %d\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port), ntohs(serv_config_addr.sin_port));
    
    // receive info from clients and responds accordingly
    while(1) {
        if((recv_len = recvfrom(s_clients, &info, sizeof(info), 0, (struct sockaddr *) &client_addr, (socklen_t *) &client_len)) == -1) {
            perror("Error on recvfrom");
        }

        printf("Server IP: %s === Port: %d\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
        printf("Client IP: %s === Port: %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        if (strcmp(info.behaviour, "autentication") == 0){
            autentication();

        } else if (strcmp(info.behaviour, "client_server") == 0){
            client_server();

        } else if (strcmp(info.behaviour, "p2p") == 0){
            p2p();
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
        sendto(s_clients, &reply, sizeof(reply), 0, (struct sockaddr *) &client_addr, (socklen_t ) client_len);
        return;
    }


    inet_ntop(AF_INET, &(client_addr.sin_addr), oldIP, INET_ADDRSTRLEN);
    printf("oldIP: %s\n", oldIP);

    // change ip to client2
    inet_pton(AF_INET, IP_dest, &(client_addr.sin_addr));
    printf("Server address2: %s\n", inet_ntoa(client_addr.sin_addr));

    // sends message to client2
    sendto(s_clients, &info, sizeof(info), 0, (struct sockaddr *) &client_addr, (socklen_t ) client_len);
    printf("Message successufly send to client with IP: %s! %s\n", IP_dest, info.message);

    // sets ip back to old
    inet_pton(AF_INET, oldIP, &(client_addr.sin_addr));
    printf("Server address3: %s\n", inet_ntoa(client_addr.sin_addr));


    // send confirmation of success
    strcpy(reply.message, "Message send successufly");
    sendto(s_clients, &reply, sizeof(reply), 0, (struct sockaddr *) &client_addr, (socklen_t ) client_len);

}


void p2p() {
    char *IP_dest;
    user_info dest_info;

    // get client2 ip
    IP_dest = find_user_ip(info.userName_dest);
    printf("IP_dest: %s\n", IP_dest);

    strcpy(info.behaviour, "info");

    if (strcmp(IP_dest, "not found") == 0) {
        strcpy(dest_info.user_IP, "not found");
        
    } else {
        // gets client info
        strcpy(dest_info.user_IP, IP_dest);
    }

    sendto(s_clients, &dest_info, sizeof(dest_info), 0, (struct sockaddr *) &client_addr, (socklen_t ) client_len);

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
    sendto(s_clients, &info, sizeof(info), 0, (struct sockaddr *) &client_addr, (socklen_t ) client_len);
}


int find_user(char userName[MAX_INFO], char password[MAX_INFO]) {
    for (int i = 0; i< n_users; i++) {
        if (strcmp(userName, users[i].userName) == 0 && strcmp(password, users[i].password) == 0) {
                return i;
        }
    }
    return -1;
}


bool valid_ip(char* ip){
    int n_p = 0, n_v = 0, i = 0;

    while(ip[i] != 0){
        if(ip[i] == '.' && (n_v == 0 || n_v > 3)) return false;

        if (isdigit(ip[i])) n_v++;

        else if (ip[i] == '.') {
            n_p++; 
            n_v = 0;
        }

        i++;
    }

    return (n_p == 3 && (n_v >= 1 && n_v <= 3));
}


char *find_user_ip(char userName[MAX_INFO]) {
    for (int i = 0; i < n_users; i++) {
        if (strcmp(userName, users[i].userName) == 0) {
            return users[i].ip;
        }
    }
    return "not found";

}


int get_info(char *str){
    char *tok;
    user_info user;
    int count = 0;
    char aux[MAX_LINE];
    strcpy(aux,str);
    tok = strtok(aux," ");
    strcpy(user.userName,tok);
    //printf("%s\n", user.userName);
    for(count = 0; tok != NULL;){
        tok = strtok(NULL, " ");

        if(tok != NULL){
            if((count == 0) && valid_ip(tok)) strcpy(user.ip,tok);
            else if(count == 1) strcpy(user.password,tok);
            else if(count == 2){
                if((tok[0] != '0' && tok[0] != '1') ||(tok[1] != '0'&&tok[1] != '1')||(tok[2] != '0'&&tok[2]!='1')){
                  printf("ERROR wrong format\n");
                  return 0;
                }
                user.client_server = tok[0] - '0';
                user.p2p = tok[1] - '0';
                user.group = tok[2] - '0';
            }
            else{
              printf("ERROR wrong format\n");
              return 0;
            }
            count ++;
        }
    }

    if(count != 3){
        printf("%d\n", count);
        printf("ERROR wrong format\n");
        return 0;
        
    }else{
        for(int i=0; i< n_users; i++){
            if(strcmp(user.userName,users[i].userName) == 0){
                printf("Username %s is already in use\n",user.userName);
                return 0;
            }
        }
        users[n_users] = user;
        n_users++;
    }

    return 1;

}


void load_info(){
    char line[MAX_LINE];
    FILE *fp;
    
    fp = fopen(file_name, "r");

    if (fp == NULL)
        printf("Error opening file %s\n", file_name);

    while(fgets(line, MAX_LINE, fp) != NULL) {
            get_info(line);
            printf("%s\n", line);
    }
    
    fclose(fp);
}


void printa(user_info *arr){
    for(int i =0; i<n_users; i++){
        printf("User %d:\n\tuserName = %s\n\tip = %s\n\tpassword = %s\n\tclient-server = %d\n\tp2p = %d\n\tgrupo = %d\n\n",
        i, arr[i].userName,arr[i].ip,arr[i].password,arr[i].client_server,arr[i].p2p,arr[i].group);
    }
}


void write_on_file(char *str){
    FILE *fp;
    fp = fopen(file_name,"a");
    if(fp == NULL) printf("ERROR: CANNOT OPEN FILE\n");
    fprintf(fp,"%s\n",str);
    fclose(fp);
}


int find_username(char name[]){
    for (int i = 0; i< n_users; i++) {
        if (strcmp(name, users[i].userName) == 0) return i;
    }
    return -1;
}


void trim(char* str){
    int count = 0; 
    int word = 0;

    for (int i = 0; str[i]; i++) 
        if (str[i] != ' ') {
            str[count++] = str[i]; 
            word = 1;
        } else if (str[i] == ' ' && word == 1) {
            str[count++] = str[i];
            word = 0;
        }

    if (word == 0) count--;
    str[count] = '\0'; 
}


void remove_from_file(char name[]){
    char aux[n_users][MAX_LINE];
    char line[MAX_LINE];
    FILE *fp;
    int i = 0;
    char *tok;
    char aux2[MAX_LINE];

    fp = fopen(file_name, "r");
    if(fp == NULL){
        printf("ERROR: CANNOT OPEN FILE\n");
        return;
    }

    while(fgets(line, MAX_LINE, fp) != NULL){

        strcpy(aux2,line);
        tok = strtok(aux2, " ");
        if(strcmp(tok,name) == 0){
            continue;
        }
        strcpy(aux[i],line);
        i++;
        printf("%s\n", aux[i-1]);
    }
    fclose(fp);
    
    fp = fopen(file_name,"w");
    if(fp == NULL){
        printf("ERROR: CANNOT OPEN FILE\n");
        return;
    }

    for(int j =0; j<i;j++){
        fputs(aux[j],fp);
    }
    fclose(fp);

}


void split(char *input, char *st, char separator){
    if(input[0] == 0) return;

    char string[strlen(input)];
    int i=0,j=0;
    while((input[i] != separator) && input[i] != 0){
      st[i] = input[i];
      i++;
    }
    st[i] =0;
    while(input[i] == separator) i++;
    while(input[i] != 0) string[j++] = input[i++];

    string[j] = 0;
    strcpy(input,string);
}


int count_words(char *str) {
    int count = 0;
    int state = 0;

    for (int i = 0; i < strlen(str); i++) {
        if (str[i] == ' ') {
            state = 0;

        } else if (state == 0 && str[i] != ' ') {
            state = 1;
            count++;
        }
    }

    return count;
}


void *config_users(void* i) {
    printf("Thread initialized\n");

    char st[MAX_LINE];
    int conf = 0, nread = 0;
    char line[MAX_LINE];
    char reply[MAX_TEXT];
    int wc = 0;

    while(1) {
        // accept connection, if an error occurs ends thread config
        conf = accept(fd_tcp,(struct sockaddr *) &serv_config_addr,(socklen_t *)&config_size);
        if (conf <= 0) return NULL;
        
        while(1) {
            // reads line from socket
            nread = read(conf, line, MAX_LINE-1);

            if (nread != -1) {
                line[nread-1] = 0;
                printf("Received config: %s\n", line);
                
                // trim string
                trim(line);

                // number of words
                wc = count_words(line);

                // get's first word
                split(line, st, ' ');

                if (strcmp(st, "QUIT") == 0) {
                    close(conf);
                    break;

                } else if(strcmp(st, "LIST") == 0){
                    for(int i=0; i < n_users ; i++ ){
                        
                        sprintf(reply, "UserName => %s\nIP => %s\nPassword => %s\nClient-Server => %d\nP2p => %d\nGrupo => %d\n\n",
                        users[i].userName,users[i].ip,users[i].password,users[i].client_server,users[i].p2p,users[i].group);
                        write(conf, reply, 1 + strlen(reply));
                    }

                } else if(strcmp(st,"ADD") == 0){
                    if(wc == 5) {
                        if(!get_info(line)) continue;
                        write_on_file(line);

                        sprintf(reply, "User added successfully\n");
                        write(conf, reply, 1 + strlen(reply));

                    } else {
                        sprintf(reply, "Wrong format: ADD <UserName> <IP> <Password> <Cliente-Servidor> <P2P> <Grupo> (Ex: ADD test 193.136.212.129 test 110)\n");
                        write(conf, reply, 1 + strlen(reply));
                    }

                } else if(strcmp(st,"DEL") == 0){
                    if(wc == 2) {
                        int x = find_username(line);
                        if(x != -1){
                            for(; x< n_users; x++){
                                users[x] = users[x+1];
                            }
                            n_users --;
                            remove_from_file(line);
                            sprintf(reply, "User removed\n");

                        } else{
                            sprintf(reply, "User not found\n");
                        }
                    } else {
                        sprintf(reply, "Wrong format: DEL <User-id>\n");
                        
                    }
                    write(conf, reply, 1 + strlen(reply));

                } else{
                    sprintf(reply, "command unknown\n");
                    write(conf, reply, 1 + strlen(reply));
                }

            } else {
                perror("ERROR");
            }

        }
    }
}
