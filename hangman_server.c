#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
#include <arpa/inet.h> 
#include <pthread.h>

#define MAX_CONNECTIONS 3
#define WORDS_COUNT 15
#define WORD_LENGTH 10
#define MAX_GUESSES 6

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int activeConnections = 0;

void error(const char *msg) {
    perror(msg);
    exit(1);
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

bool updateProgress(char *trueWord, char *progress, char guess) {
    bool found = false;
    for (int i = 0; i < strlen(trueWord); i++) {
        if (guess == trueWord[i] && progress[i] == '_') {
            progress[i] = guess;
            found = true;
        }
    }
    return found;
}

void* handleClient(void* socketDescriptor) {
    int newsockfd = *(int*)socketDescriptor;
    free(socketDescriptor);
    struct dataClient dataClient;
    struct dataServer dataServer;
    
    read(newsockfd, &dataClient, sizeof(dataClient));
    dataServer.flag = 0;
    strcpy(dataServer.data, "nope");
    write(newsockfd, &dataServer, sizeof(dataServer));
    read(newsockfd, &dataClient, sizeof(dataClient));
    
    if (dataClient.msgLength != 0) {
        dataServer.flag = 10;
        write(newsockfd, &dataServer, sizeof(dataServer));
        pthread_mutex_lock(&lock);
        activeConnections--;
        pthread_mutex_unlock(&lock);
        close(newsockfd);
        return NULL;
    }

    FILE *file = fopen("hangman_words.txt", "r");
    if (!file) {
        error("Error opening file");
    }
    
    char words[WORDS_COUNT][WORD_LENGTH];
    for (int i = 0; i < WORDS_COUNT; i++) {
        fgets(words[i], WORD_LENGTH, file);
        strtok(words[i], "\n");
    }
    fclose(file);

    char trueWord[WORD_LENGTH];
    strcpy(trueWord, words[rand() % WORDS_COUNT]);
    dataServer.len = strlen(trueWord);

    char progress[dataServer.len];
    for(int i = 0; i < dataServer.len; i++) {
        progress[i] = '_';
    }
    char incorrectGuesses[MAX_GUESSES] = {0};
    dataServer.inc = 0;
    
    strncpy(dataServer.data, progress, dataServer.len);
    write(newsockfd, &dataServer, sizeof(dataServer));
    read(newsockfd, &dataClient, sizeof(dataClient));
    
    
    while(1) {
        if(dataClient.msgLength == -1) {
            activeConnections--;
            close(newsockfd);
            return NULL;
        }
        if(dataClient.msgLength != 1 || !isalpha(dataClient.data[0])) {
            dataServer.flag = 31;
            strcpy(dataServer.data, ">>>Error! Please guess one letter.");
            write(newsockfd, &dataServer, sizeof(dataServer));
            read(newsockfd,&dataClient, sizeof(dataClient));
            continue;
        }

        bool correct = updateProgress(trueWord, progress, dataClient.data[0]);
        if(correct) {
            if (strncmp(progress, trueWord, dataServer.len) == 0) {
               
                dataServer.flag = 8;
                strcpy(dataServer.data, "You Win!");
                break;
            } 
            
        } else {
            incorrectGuesses[dataServer.inc++] = dataClient.data[0];
            
            if(dataServer.inc == 6) {
               
                dataServer.flag = 9;
                strcpy(dataServer.data, "You Lose!");
                break;
            }
        }

        dataServer.flag = 0;
        bzero(dataServer.data, 256);
        strncpy(dataServer.data, progress, dataServer.len);
        strcat(dataServer.data, incorrectGuesses);
        write(newsockfd, &dataServer, sizeof(dataServer));
        read(newsockfd,&dataClient, sizeof(dataClient));
    }


    strncat(dataServer.data, trueWord, dataServer.len);
    pthread_mutex_lock(&lock);
    write(newsockfd, &dataServer, sizeof(dataServer));
    close(newsockfd);
    pthread_mutex_unlock(&lock);
    activeConnections--;
    
    return NULL; 
}

void rejectClient(int newsockfd) {
    struct dataServer dataServer;
    dataServer.flag = -1;
    write(newsockfd, &dataServer, sizeof(dataServer));
    close(newsockfd);
}

int main() {
    srand(time(NULL));
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");

    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(8080);

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(sockfd, MAX_CONNECTIONS);
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    
    while (1) {
        int *newsockfd_ptr = malloc(sizeof(int));
        *newsockfd_ptr = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
        if (*newsockfd_ptr < 0) error("ERROR on accept");

        pthread_mutex_lock(&lock);
        if (activeConnections >= MAX_CONNECTIONS) {
            pthread_mutex_unlock(&lock);
            rejectClient(*newsockfd_ptr);
            free(newsockfd_ptr);
        } else {
            activeConnections++;
            pthread_mutex_unlock(&lock);
            pthread_t thread;
            pthread_create(&thread, NULL, handleClient, newsockfd_ptr);
            pthread_detach(thread);
        }
    }

    close(sockfd);
    return 0;
}




