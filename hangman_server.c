#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>

#define MAX_CLIENTS 3
#define MAX_WORDS 15
#define MAX_LENGTH 10

int activeConnections = 0;

void handleError(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

struct ServerMessage {
    short status;
    short wordLength;
    short incorrect;
    char content[256];
};

struct ClientMessage {
    int length;
    char content[256];
};

bool updateProgress(char* original, char* progress, char guess) {
    int len = strlen(original);
    bool found = false;
    for (int i = 0; i < len; i++) {
        if (original[i] == guess && progress[i] == '_') {
            progress[i] = guess;
            found = true;
        }
    }
    return found;
}

void* handleClient(void* socketDescriptor) {
    int clientSocket = *(int*)socketDescriptor;
    struct ClientMessage clientMsg;
    struct ServerMessage serverMsg;
    
    read(clientSocket, &clientMsg, sizeof(clientMsg));
    serverMsg.status = 0;
    strcpy(serverMsg.content, "nope");
    write(clientSocket, &serverMsg, sizeof(serverMsg));
    read(clientSocket, &clientMsg, sizeof(clientMsg));

    if (clientMsg.length != 0) {
        serverMsg.status = 10;
        write(clientSocket, &serverMsg, sizeof(serverMsg));
        activeConnections--;
        close(clientSocket);
        return NULL;
    }

    FILE *wordFile = fopen("hangman_words.txt", "r");
    char words[MAX_WORDS][MAX_LENGTH];
    char chosenWord[MAX_LENGTH];

    for (int i = 0; i < MAX_WORDS; i++) {
        fgets(words[i], MAX_LENGTH, wordFile);
    }
    fclose(wordFile);

    srand(time(NULL));
    strcpy(chosenWord, words[rand() % MAX_WORDS]);
    serverMsg.wordLength = strlen(chosenWord);
    
    if (chosenWord[serverMsg.wordLength - 1] == '\n') {
        serverMsg.wordLength--;
    }

    char displayedWord[serverMsg.wordLength];
    memset(displayedWord, '_', serverMsg.wordLength);
    char wrongGuesses[6] = {0};
    serverMsg.incorrect = 0;

    strncpy(serverMsg.content, displayedWord, serverMsg.wordLength);
    write(clientSocket, &serverMsg, sizeof(serverMsg));
    read(clientSocket, &clientMsg, sizeof(clientMsg));
    
    bool won;
    while (1) {
        if (clientMsg.length == -1) {
            activeConnections--;
            close(clientSocket);
            return NULL;
        }
        if (clientMsg.length != 1 || !isalpha(clientMsg.content[0])) {
            serverMsg.status = 31;
            strcpy(serverMsg.content, ">>>Error! Guess one letter.");
            write(clientSocket, &serverMsg, sizeof(serverMsg));
            read(clientSocket, &clientMsg, sizeof(clientMsg));
            continue;
        }

        if (updateProgress(chosenWord, displayedWord, clientMsg.content[0])) {
            if (strncmp(displayedWord, chosenWord, serverMsg.wordLength) == 0) {
                won = true;
                break;
            }
        } else {
            wrongGuesses[serverMsg.incorrect] = clientMsg.content[0];
            serverMsg.incorrect++;
            if (serverMsg.incorrect == 6) {
                won = false;
                break;
            }
        }

        serverMsg.status = 0;
        snprintf(serverMsg.content, sizeof(serverMsg.content), "%s%s", displayedWord, wrongGuesses);
        write(clientSocket, &serverMsg, sizeof(serverMsg));
        read(clientSocket, &clientMsg, sizeof(clientMsg));
    }

    serverMsg.status = won ? 8 : 9;
    snprintf(serverMsg.content, sizeof(serverMsg.content), "%s%s", won ? "You Win! " : "You Lose! ", chosenWord);
    write(clientSocket, &serverMsg, sizeof(serverMsg));
    close(clientSocket);
    activeConnections--;
    return NULL;
}

void rejectClient(int socket) {
    struct ServerMessage serverMsg;
    serverMsg.status = -1;
    write(socket, &serverMsg, sizeof(serverMsg));
    close(socket);
}

int main() {
    int serverSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    pthread_t clientThreads[MAX_CLIENTS];

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) handleError("Socket creation failed");
    
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(8080);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        handleError("Binding failed");
    }
    
    listen(serverSocket, MAX_CLIENTS);
    while (1) {
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSocket < 0) handleError("Accept failed");
        
        if (activeConnections >= MAX_CLIENTS) {
            rejectClient(clientSocket);
        } else {
            activeConnections++;
            pthread_create(&clientThreads[activeConnections - 1], NULL, handleClient, (void*)&clientSocket);
        }
    }
    return 0;
}
