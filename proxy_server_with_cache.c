#include "proxy_parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>

typedef struct lru lru;
#define MAX_CLIENTS 10

struct lru{
    char* data;
    int len;
    char* url;
    time_t lru_time;
    lru* next;
};

lru* find(char *url);
int add_element(char *data,int len,char *url);
void remove();

int port_number = 8080;
int proxy_socketid;
pthread_t tid[MAX_CLIENTS];
sem_t sem;
pthread_mutex_t lock;

lru *head;
int lru_size;

int main(int argc, char *argv[]){
    int client_socket_id,client_len;
    struct sockaddr_in server_addr,client_addr;
    sem_init(&sem,0,MAX_CLIENTS);
    pthread_mutex_init(&lock,NULL);

    if(argv==2){
        port_number = atoi(argv[0]);
    }else{
        printf("Port number not present in Args.\n");
        return 0;
    }

    print("Starting Server at %d",port_number);
    // AF_INET -> IPv4
    // SOCK_STREAM -> Sequenced, reliable, connection-based byte streams.
    // 0-> for single protocol
    proxy_socketid = socket(AF_INET,SOCK_STREAM,0); // returns fd of socket.
    if(proxy_socketid<0){
        perror("Failed to Create a Server \n");
        exit(1);
    }
    int reuse =1 ;
    if(setsockopt(proxy_socketid,SOL_SOCKET,SO_REUSEADDR,(const int*)reuse,sizeof(reuse))<0){
        perror("SetSockOpt filled. \n");
        exit(1);
    }

    bzero((char *)&server_addr,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number); // convert portnumber to Network byte order(Big Endian)
    // since the host system is running on Intel Iris(CISC processor).
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(proxy_socketid,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
        perror("Bind Failed \n");
        exit(1);
    }

    print("Binding a port %d \n", port_number);
    int listen_status = listen(proxy_socketid,MAX_CLIENTS);

    if(listen_status<0){
        perror("Some issue in listening \n");
        exit(1);
    }
    int i=0;
    int connected_sockid[MAX_CLIENTS];

    while(1){
        bzero((char *)&client_addr,sizeof(client_addr));
        int client_len = sizeof(client_addr);
        client_socket_id = accept(proxy_socketid,(struct sockaddr *)&client_addr,(socklen_t)&client_len);
        if(client_socket_id<0){
            perror("Unable to accept \n");
            exit(1);
        }
        else{
            connected_sockid[i] = client_socket_id;
        }

        struct sockaddr_in *client_pt = (struct sockaddr_in *)&client_addr;

        struct in_addr ip_addr = client_pt->sin_addr;

        char str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET,&ip_addr,str,INET_ADDRSTRLEN);
        print("Client is connected with portNumber %d and ip address is %s\n",ntohs(client_addr.sin_port),str);

        
        return 0;
    }
}