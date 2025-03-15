//Daniel Hwang and Nevin Manimaran
//CS176a Homework 5b
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h> 
#include <netinet/in.h>
void error(const char *message){
    perror(message);
    exit(EXIT_FAILURE);
}
struct serverData{
    short status;
    short dataLength;
    short increment;
    char buffer[256];
};
struct clientData{
    int messageLength; 
    char buffer[256];
};
int main(int argc, char *argv[]){
    int sockDescriptor;
    int portNum;
    int n;
    struct sockaddr_in serverAddr;
    struct hostent *server;
    char buffer[256];
    if(3 >argc){
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    portNum = atoi(argv[2]);
    sockDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (sockDescriptor < 0)
        error("ERROR with socket");

    server = gethostbyname(argv[1]);
    if(!server){
        fprintf(stderr, "ERROR: Server not found\n");
        exit(EXIT_FAILURE);
    }
    bzero((char *) &serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serverAddr.sin_addr.s_addr,
         server->h_length);
    serverAddr.sin_port = htons(portNum);
    if (connect(sockDescriptor,(struct sockaddr *) &serverAddr,sizeof(serverAddr)) < 0) 
        error("ERROR connecting");
    
    struct clientData clientData;
    strcpy(clientData.buffer, "nope client");
    n = write(sockDescriptor,&clientData, sizeof(clientData));

    struct serverData serverData;
    n = read(sockDescriptor, &serverData, sizeof(serverData));
    //If a fourth client (player) tries to connect to the game-server, the server should send a
    //“server-overloaded ” message then gracefully end the connection.
    if(serverData.status == -1){
        printf(">>>server-overloaded\n");
        return 0;
    }
    //If the user says no terminate client otherwise send an empty message to the server
    //with msg length = 0. (more on this below) signaling game start.
    memset(&clientData, 0, sizeof(clientData));  
    strcpy(clientData.buffer, "");  
    clientData.messageLength = 0;
    printf(">>>Ready to start game? (y/n): ");
    fgets(buffer, 255, stdin);

    if (buffer[0] != 'y') {
        clientData.messageLength = 1;
    }else{
        clientData.messageLength = 0;
    }
    strcpy(clientData.buffer, buffer);
    n=write(sockDescriptor, &clientData, sizeof(clientData));
    if (n < 0)
        error("ERROR writing to socket");

    if (buffer[0] != 'y') {
        puts("");
        return 0;
    }

    n = read(sockDescriptor, &serverData, sizeof(serverData));
    while (serverData.status == 0 || serverData.status == 31) {
        if (serverData.status == 31) {
            printf("%s\n", serverData.buffer);
        } else {
            printf(">>>");
            for (int i = 0; i < serverData.dataLength - 1; i++) {
                printf("%c ", serverData.buffer[i]);
            }
            printf("%c", serverData.buffer[serverData.dataLength-1]);
            puts("");
            printf(">>>Incorrect Guesses: ");
            for (int i = 0; i < serverData.increment-1; i++) {
                printf("%c ", serverData.buffer[i+serverData.dataLength]);
            }
            if (serverData.increment > 0) {
                printf("%c", serverData.buffer[serverData.increment+serverData.dataLength-1]);
            }
            puts("");

        }
        printf(">>>Letter to guess: ");
        memset(clientData.buffer,0,256);
        fgets(clientData.buffer,255,stdin);
        clientData.messageLength=strlen(clientData.buffer)-1;

        n = write(sockDescriptor, &clientData, sizeof(clientData));
        if (n < 0) {
            error("ERROR writing to socket");
        }
        n = read(sockDescriptor, &serverData, sizeof(serverData));
    }
    //If the player instead guesses 6 incorrect letters and loses the game, replace “You Win!” with
    //“You Lose.”
    printf(">>>The word was %s\n", &serverData.buffer[serverData.status]);
    printf(">>>%.*s\n", serverData.status, serverData.buffer);
    printf(">>>Game Over!\n");
    return 0;
}
