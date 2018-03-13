/*
 * COMP30023 Computer Systems
 * Semester 1 2017
 * Project 2
 * Server with Proof of Work Solver
 *
 * University Username: dyeoh
 * Name: Darren Yeoh Cheang Leng
 * Student ID: 715863
 *
 * This module handles the functionality of 
 * the server-based proof of work solver.
 *
 * Modified from the server code provided in the workshops.
 *
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include "sol.c"
#include "work.c"

#define PING_HEADER "PING"
#define PONG_HEADER "PONG"
#define OKAY_HEADER "OKAY"
#define ERRO_HEADER "ERRO"
#define SOLN_HEADER "SOLN"
#define WORK_HEADER "WORK"
#define ABRT_HEADER "ABRT"
#define LOG_FILE "log.txt"
#define DELIM "\r\n"
#define MAX_MESSAGE_LENGTH 256
#define MAX_BUFFER_LENGTH 512
#define MAX_CLIENTS 100
#define HEADER_SIZE 4
#define MAX_ERRO_LENGTH 40
#define MAX_SOL_LENGTH 97
#define MAX_HEADER_LENGTH 6

/*Struct for passing multiple parameters into a client thread*/
struct readThreadParams {
    int new_sock;
	char ip[INET6_ADDRSTRLEN];
	char message[MAX_MESSAGE_LENGTH];
};

/*Global variables*/
work_t *work_queue = NULL;
pthread_t workthread;
pthread_mutex_t lock;

/*Function prototypes*/
void *connection_handler(void *arg_struct);
void message_handler(int sock, char buffer[MAX_BUFFER_LENGTH], char ip[INET6_ADDRSTRLEN]);
void *work_handler();

/*Server implementation with multiple client handling*/
int main(int argc, char **argv) {
	time_t rawtime;
	struct tm *timeinfo;
	int listenfd, clientfd, portno, clilen;
	int *clifdptr, optval;
	struct sockaddr_in serv_addr, cli_addr;
	FILE *logFile;
	
	remove(LOG_FILE);
	if (argc < 2) 
	{
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
	}

	/*Creates TCP socket*/
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	
	if (listenfd < 0) 
	{
		perror("ERROR opening socket");
		exit(1);
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));

	portno = atoi(argv[1]);
	
	/*Creates address the server listens on*/
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	
	/*Sets the socket options and starts listening*/
	optval = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));
	if (bind(listenfd, (struct sockaddr *) &serv_addr,
			sizeof(serv_addr)) < 0) 
	{
		perror("ERROR on binding");
		exit(1);
	}

	/*Write server started to log file*/
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	logFile = fopen(LOG_FILE , "ab+");
	fprintf(logFile, "[%.24s] [server 0.0.0.0] Server started.\n", asctime (timeinfo));
	fprintf(stdout,"Server started successfully on port %d, listening for connections, a more detailed log file will be created in the directory.\n", portno);
	fclose(logFile);
	
	listen(listenfd, MAX_CLIENTS);
	clilen = sizeof(cli_addr);

	/*Starst the worker thread*/
	if(pthread_create(&workthread, NULL, work_handler, (void*)NULL) < 0){
		perror("failed to create work thread");
		exit(1);
	}
	
	/*Handles incoming connections*/
	while( (clientfd = accept(listenfd, (struct sockaddr *) &cli_addr, &clilen)) ){
		struct readThreadParams readParams;
        pthread_t thread;
        clifdptr = malloc(1);
        *clifdptr = clientfd;
		readParams.new_sock = clientfd;
		
		inet_ntop(AF_INET, (struct sockaddr *)&cli_addr.sin_addr,
		readParams.ip, sizeof readParams.ip);
		
		logFile = fopen(LOG_FILE , "ab+");
		
		timeinfo = localtime ( &rawtime );
		fprintf(logFile, "[%.24s] [server 0.0.0.0] Incoming connection from client %s\n", asctime (timeinfo), readParams.ip);
		fclose(logFile);
		
        if( pthread_create( &thread , NULL ,  connection_handler ,  (void*)&readParams) < 0)
        {
            perror("could not create thread");
            exit(1);
        }
	}

	if(clientfd < 0)
    {
        perror("accept failed");
        return 1;
    }
	return 0;
}


/* This will handle connections for each client*/
void *connection_handler(void *arg_struct){
	time_t rawtime;
	struct tm *timeinfo;
	struct readThreadParams *readParams = arg_struct;
    //Get the socket descriptor
    int sock = readParams->new_sock;
    char *message , buffer[MAX_BUFFER_LENGTH];
	FILE *logFile;
	
	time ( &rawtime );
	
	
	logFile = fopen(LOG_FILE , "ab+");
	
	timeinfo = localtime ( &rawtime );
	fprintf(logFile, 
	"[%.24s] [client %s] [sockid: %d] Successfully connected to server.\n", 
	asctime (timeinfo), readParams->ip, sock);
	
	fclose(logFile);
	printf("Client %s connected\n", readParams->ip);
	
	bzero(buffer,MAX_BUFFER_LENGTH);
	
	while(recv(sock, buffer, MAX_BUFFER_LENGTH, 0) > 0){
		message_handler(sock, buffer, readParams->ip);
		bzero(buffer,MAX_BUFFER_LENGTH);
	}
	
	printf("Client %s disconnected\n", readParams->ip);
	
	close(sock);
     
    return 0;
}

/*Handles the messages sent from a client*/
void message_handler(int sock, char buffer[MAX_BUFFER_LENGTH], char ip[INET6_ADDRSTRLEN]){
	/*System time variables*/
	time_t rawtime;
	struct tm *timeinfo;
	
	char *token;
	FILE *logFile;
	
	token = strtok(buffer, DELIM);
	
	while( token != NULL ) {
		if(strcmp(token, PING_HEADER) == 0){
			/*Logs the message received from client*/
			time(&rawtime);
			timeinfo = localtime ( &rawtime );
			logFile = fopen(LOG_FILE , "ab+");
			fprintf(logFile, "[%.24s] [client %s] [sockid: %d] Sent PING to server\n", asctime (timeinfo), ip, sock);
			
			/*Send reply*/
			send(sock, "PONG\r\n", MAX_HEADER_LENGTH, MSG_NOSIGNAL);
			
			/*Logs the message sent from server*/
			time(&rawtime);
			timeinfo = localtime ( &rawtime );
			fprintf(logFile, "[%.24s] [server 0.0.0.0] Sent PONG to client %s sockid: %d\n", asctime (timeinfo), ip, sock);
			fclose(logFile);
		}
		else if(strcmp(token, PONG_HEADER) == 0){
			/*Logs the message received from client*/
			time(&rawtime);
			timeinfo = localtime ( &rawtime );
			logFile = fopen(LOG_FILE , "ab+");
			fprintf(logFile, "[%.24s] [client %s] [sockid: %d] Sent PONG to server\n", asctime (timeinfo), ip, sock);
			
			/*Sends the reply*/
			send(sock, "ERRO: PONG reserved for server        \r\n", MAX_ERRO_LENGTH, MSG_NOSIGNAL);
			
			/*Logs the message sent from server*/
			time(&rawtime);
			timeinfo = localtime ( &rawtime );
			fprintf(logFile, "[%.24s] [server 0.0.0.0] Sent ERRO: Pong reserved for server to client %s sockid: %d\n", asctime (timeinfo), ip, sock);
			fclose(logFile);
		}
		else if(strcmp(token, OKAY_HEADER) == 0){
			/*Logs the message received from client*/
			time(&rawtime);
			timeinfo = localtime ( &rawtime );
			logFile = fopen(LOG_FILE , "ab+");
			fprintf(logFile, "[%.24s] [client %s] [sockid: %d] Sent OKAY message to server\n", asctime (timeinfo), ip, sock);
			
			/*Sends the reply*/
			send(sock, "ERRO: Not okay to send OKAY           \r\n", MAX_ERRO_LENGTH, MSG_NOSIGNAL);

			/*Logs the message sent from server*/
			time(&rawtime);
			timeinfo = localtime ( &rawtime );
			fprintf(logFile, "[%.24s] [server 0.0.0.0] Sent ERRO: Not okay to send okay to client %s sockid: %d\n", asctime (timeinfo), ip, sock);
			fclose(logFile);
		}
		else if(strncmp(token, ERRO_HEADER, HEADER_SIZE) == 0){
			/*Logs the message received from client*/
			time(&rawtime);
			timeinfo = localtime ( &rawtime );
			logFile = fopen(LOG_FILE , "ab+");
			fprintf(logFile, "[%.24s] [client %s] [sockid: %d] Sent ERRO to server\n", asctime (timeinfo), ip, sock);
			
			/*Sends the reply*/
			send(sock, "ERRO: Not okay to send ERRO           \r\n", MAX_ERRO_LENGTH, MSG_NOSIGNAL);
			
			/*Logs the message sent from server*/
			time(&rawtime);
			timeinfo = localtime ( &rawtime );
			fprintf(logFile, "[%.24s] [server 0.0.0.0] Sent ERRO to client %s sockid: %d\n", asctime (timeinfo), ip, sock);
			fclose(logFile);
		}
		else if(strncmp(token, SOLN_HEADER, HEADER_SIZE) == 0){
			/*Logs the message received from client*/
			time(&rawtime);
			timeinfo = localtime ( &rawtime );
			logFile = fopen(LOG_FILE , "ab+");
			fprintf(logFile, "[%.24s] [client %s] [sockid: %d] Sent %s to server\n", asctime (timeinfo), ip, sock, token);
			fclose(logFile);
			
			/*Verifies the solution and sends back the appropriate message*/
			if(verify_solution(token) == 1){
				send(sock, "OKAY\r\n", MAX_HEADER_LENGTH, MSG_NOSIGNAL);
				
				/*Logs the message sent from server*/
				time(&rawtime);
				timeinfo = localtime(&rawtime);
				logFile = fopen(LOG_FILE , "ab+");
				fprintf(logFile, "[%.24s] [server 0.0.0.0] Sent OKAY to client %s sockid: %d\n", asctime (timeinfo), ip, sock);
				fclose(logFile);
			}
			else{
				send(sock, "ERRO: Invalid/malformed solution      \r\n", MAX_ERRO_LENGTH, MSG_NOSIGNAL);
				
				/*Logs the message sent from server*/
				time(&rawtime);
				timeinfo = localtime(&rawtime);
				logFile = fopen(LOG_FILE , "ab+");
				fprintf(logFile, "[%.24s] [server 0.0.0.0] Sent ERRO:Invalid solution to client %s sockid: %d\n", asctime (timeinfo), ip, sock);
				fclose(logFile);
			}
			
		}
		else if(strncmp(token, WORK_HEADER, HEADER_SIZE) == 0){
			/*Logs the message received from client*/
			time(&rawtime);
			timeinfo = localtime ( &rawtime );
			logFile = fopen(LOG_FILE , "ab+");
			fprintf(logFile, "[%.24s] [client %s] [sockid: %d] Sent %s to server\n", asctime (timeinfo), ip, sock, token);
			fclose(logFile);
			
			/*Adds the work to the queue*/
			work_t *new_work = initWork(token, sock, ip);
			work_queue = insertWork(&work_queue, new_work);
			
		}
		else if(strcmp(token, ABRT_HEADER) == 0){
			//printf("ABORTION IN PROGRESS\n");
			
			send(sock, "OKAY\r\n", MAX_HEADER_LENGTH, MSG_NOSIGNAL);
			time(&rawtime);
			timeinfo = localtime ( &rawtime );
			logFile = fopen(LOG_FILE , "ab+");
			fprintf(logFile, "[%.24s] [client %s] [sockid: %d] Sent ABRT to server\n", asctime (timeinfo), ip, sock);
			fclose(logFile);
			pthread_cancel(workthread);
			if(pthread_create(&workthread, NULL, work_handler, (void*)NULL) < 0){
				perror("failed to create work thread");
				exit(1);
			}
			deleteKey(&work_queue, sock);
			
			
			time(&rawtime);
			timeinfo = localtime ( &rawtime );
			logFile = fopen(LOG_FILE , "ab+");
			fprintf(logFile, "[%.24s] [server 0.0.0.0] Sent OKAY to client %s sockid: %d\n", asctime (timeinfo), ip, sock);
			fclose(logFile);
		}
		else{
			/*Received message from server*/
			time(&rawtime);
			timeinfo = localtime ( &rawtime );
			logFile = fopen(LOG_FILE , "ab+");
			fprintf(logFile, "[%.24s] [client %s] [sockid: %d] Sent unknown message to server\n", asctime (timeinfo), ip, sock);
			
			send(sock, "ERRO: Unknown message sent            \r\n", MAX_ERRO_LENGTH, MSG_NOSIGNAL);
			
			timeinfo = localtime ( &rawtime );
			fprintf(logFile, "[%.24s] [server 0.0.0.0] Sent ERRO: unknown message to client %s sockid: %d\n", asctime (timeinfo), ip, sock);
			fclose(logFile);
		}
	
	  token = strtok(NULL, DELIM);
	}
}

/*The thread for processing work messages from a queue*/
void *work_handler(){
	time_t rawtime;
	struct tm *timeinfo;
	FILE *logFile;
	int n;
	
	/*This ensures the thread can be terminated prematurely*/
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	
	/*Infinite loop to keep processing work messages from the queue*/
	while(1){
		if(work_queue != NULL){
			/*Locks pthread to ensure no concurrency issues arise*/
			pthread_mutex_lock(&lock);
			work_t *pop = popWork(&work_queue);
			pthread_mutex_unlock(&lock);
			
			/*Handles the popped work from the work queue*/
			n = solve(pop->work_message, pop->solution);
			if(n == 1){
				send(pop->sockid, pop->solution, MAX_SOL_LENGTH, MSG_NOSIGNAL);
				/*Logs details of solution found by work thread*/
				logFile = fopen(LOG_FILE , "ab+");
				time (&rawtime);
				timeinfo = localtime ( &rawtime );
				fprintf(logFile, "[%.24s] [server 0.0.0.0] Sent work solution back to client %s sockid: %d Solution: %s",
				asctime (timeinfo), pop->ip, pop->sockid, pop->solution);
				fclose(logFile);
			}
			else{
				send(pop->sockid, "ERRO: Malformed work message sent     \r\n", MAX_ERRO_LENGTH, MSG_NOSIGNAL);
				logFile = fopen(LOG_FILE , "ab+");
				time (&rawtime);
				timeinfo = localtime ( &rawtime );
				fprintf(logFile, "[%.24s] [server 0.0.0.0] Client %s sockid: %d entered malformed work solution\n",
				asctime (timeinfo), pop->ip, pop->sockid);
				fclose(logFile);
			}
			//printf("%u, %s", (unsigned)strlen(pop->solution), pop->solution);
			

			
		}
	}
}
