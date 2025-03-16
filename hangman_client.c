// Daniel Hwang and Nevin Manimaran
// CS176a Homework 5b
#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void error(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

struct serverData {
    short status;
    short dataLength;
    short increment;
    char data[256];
};

struct clientData {
    int messageLength;
    char data[256];
};

int main(int argc, char *argv[]) {
    int sockDescriptor;
    int portNum;
    struct sockaddr_in serverAddr;
    struct hostent *server;

    if (argc < 3) {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(0);
    }
    portNum = atoi(argv[2]);
    sockDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (sockDescriptor < 0)
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }

    bzero((char *)&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serverAddr.sin_addr.s_addr, server->h_length);
    serverAddr.sin_port = htons(portNum);
    if (connect(sockDescriptor, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        error("ERROR connecting");

    struct clientData clientData;
    strcpy(clientData.data, "nope client");
    write(sockDescriptor, &clientData, sizeof(clientData));

    struct serverData serverData;
    read(sockDescriptor, &serverData, sizeof(serverData));
    if (serverData.status == -1) {
        printf(">>>server-overloaded\n");
        return 0;
    }

    printf(">>>Ready to start game? (y/n): ");
    char buffer[256];
    bzero(buffer, 256);
    fgets(buffer, 255, stdin);

    if (buffer[0] == 'y') {
        clientData.messageLength = 0;
    } else {
        clientData.messageLength = 1;
    }
    strcpy(clientData.data, buffer);
    write(sockDescriptor, &clientData, sizeof(clientData));

    if (buffer[0] != 'y') {
        printf("\n");
        return 0;
    }

    read(sockDescriptor, &serverData, sizeof(serverData));
    while (serverData.status == 0 || serverData.status == 31) {
        if (serverData.status == 31) {
            printf("%s\n", serverData.data);
        } else {
            printf(">>>");
            printf("mycode  ");
            printf("%c ", serverData.data[0]);
            for (int i = 0; i < serverData.dataLength - 1; i++) {
                printf("%c ", serverData.data[i]);
            }
            printf("%c", serverData.data[serverData.dataLength - 1]);
            printf("\n>>>Incorrect Guesses: ");
            for (int i = 0; i < serverData.increment - 1; i++) {
                printf("%c ", serverData.data[i + serverData.dataLength]);
            }
            if (serverData.increment > 0) {
                printf("%c", serverData.data[serverData.increment + serverData.dataLength - 1]);
            }
            printf("\n");
        }

        if (serverData.status != 31) {
            printf(">>>\n");
        }

        printf(">>>Letter to guess: ");
        bzero(buffer, 256);
        if (fgets(buffer, 255, stdin) == NULL) {
            clientData.messageLength = -1;
        } else {
            buffer[0] = tolower(buffer[0]); 
            clientData.messageLength = 1;
            strncpy(clientData.data, buffer, 1); 
        }

        write(sockDescriptor, &clientData, sizeof(clientData));
        if (clientData.messageLength == -1) {
            printf("\n");
            return 0;
        }
        read(sockDescriptor, &serverData, sizeof(serverData));
    }

    printf(">>>The word was %s\n", serverData.data);
    printf(">>>Game Over!\n");

    return 0;
}
