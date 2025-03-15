
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

struct dataServer {
    short flag;
    short len;
    short inc;
    char data[256];
};

struct dataClient {
    int msgLength;
    char data[256];
};

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
    struct dataClient dataClient;
    strcpy(dataClient.data, "nope client");
    n = write(sockfd,&dataClient, sizeof(dataClient));

    struct dataServer dataServer;

    n = read(sockfd, &dataServer, sizeof(dataServer));
    if(dataServer.flag == -1) {
        printf(">>>server-overloaded\n");
        return 0;
    }

    printf(">>>Ready to start the games? (y/n): ");
    bzero(buffer,256);
    fgets(buffer,255,stdin);

    if(buffer[0] == 'y') {
        dataClient.msgLength = 0;
    } else {
        dataClient.msgLength = 1;
    }
    strcpy(dataClient.data, buffer);

    n = write(sockfd,&dataClient, sizeof(dataClient));
    if (n < 0) 
        error("ERROR writing to socket");

    if(buffer[0] != 'y') {
        printf("\n");
        return 0;
    }

    n = read(sockfd, &dataServer, sizeof(dataServer));
    while(dataServer.flag == 0 || dataServer.flag == 31) {
        if(dataServer.flag == 31) {
            printf("%s\n", dataServer.data);
        } else {
            printf(">>>");
            for(int i = 0; i < dataServer.len - 1; i++) {
                printf("%c ", dataServer.data[i]);
            }
            printf("%c", dataServer.data[dataServer.len-1]);
            printf("\n");
            printf(">>>Incorrect Guesses:  ");
            for(int i = 0; i < dataServer.inc-1; i++) {
                printf("%c ", dataServer.data[i+dataServer.len]);
            }
            if(dataServer.inc > 0) {
                printf("%c", dataServer.data[dataServer.inc+dataServer.len-1]);
            }
            printf("\n");
        }

        if(dataServer.flag != 31) {
            printf(">>>\n");
        }

        printf(">>>Pick to guess: ");
        bzero(buffer,256);
        if(fgets(buffer,255,stdin) == NULL) {
            dataClient.msgLength = -1;
        } else {
            dataClient.msgLength = strlen(buffer)-1;
        }

        bzero(dataClient.data,256);
        strcpy(dataClient.data, buffer);
        n = write(sockfd,&dataClient, sizeof(dataClient));
        if (n < 0) {
            error("ERROR writing to socket");
        } 
        if(dataClient.msgLength == -1) {
            printf("\n");
            return 0;
        }
        n = read(sockfd, &dataServer, sizeof(dataServer));
    }

    printf(">>>The word was %s\n", &dataServer.data[dataServer.flag]);
    printf(">>>%.*s\n", dataServer.flag, dataServer.data);
    

    return 0;
}