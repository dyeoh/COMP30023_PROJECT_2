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
 * This module provides utility functions needed in
 * the work queue.
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <netinet/in.h>

#define MAX_MESSAGE_LENGTH 256
#define MAX_SOL_LENGTH 97

/*Structure of work queue*/
typedef struct work{
	char work_message[MAX_MESSAGE_LENGTH];
	int sockid;
	char ip[INET6_ADDRSTRLEN];
	char solution[MAX_SOL_LENGTH];
	struct work* next;
}work_t;

/*Function prototypes*/
work_t* initWork(char *work_message, int sockid, char ip[INET6_ADDRSTRLEN]);
work_t* insertWork(work_t **head, work_t *work);
work_t* popWork(work_t **head);
void deleteKey(work_t **head, int sock);

/*int main(int argc, char **argv){
	work_t *work_queue;
	work_queue = NULL;
	work_t *test = initWork("TEST", 1, "HI");
	work_queue = insertWork(&work_queue, test);
	
	work_t *test2 = initWork("TEST", 2, "HI");
	work_queue = insertWork(&work_queue, test2);
	
	
	work_t *pop = popWork(&work_queue);
	printf("%s %d %s\n", pop->work_message, pop->sockid, pop->ip);
	
	pop = popWork(&work_queue);
	printf("%s %d %s\n", pop->work_message, pop->sockid, pop->ip);
	
	if(work_queue == NULL){
		printf("POPPING SUCCESSFUL\n");
	}
	
	return 0;
}

/*Initializes a new work*/
work_t* initWork(char *work_message, int sockid, char ip[INET6_ADDRSTRLEN]){
	work_t *temp = (struct work*) malloc(sizeof(struct work));
	strcpy(temp->work_message, work_message);
	temp->sockid = sockid;
	strcpy(temp->ip, ip);
	temp->next = NULL;
	return temp;
}

/*Inserts a new process at the end of the list*/
work_t* insertWork(work_t **head, work_t *work){
	if(*head==NULL){
		return work;
	}
	else{
		(*head)->next = insertWork(&(*head)->next, work);
		return *head;
	}
}

/*Pops the first item off the list and returns it*/
work_t* popWork(work_t **head) {
	work_t *temp = *head;
    if (temp) {
        *head = temp->next;
    }
	return temp;
}

/* Given a reference (pointer to pointer) to the head of a list and
   a key, deletes all occurrence of the given key in linked list */
void deleteKey(work_t **head, int sock)
{
    // Store head node
    work_t* temp = *head, *prev;
 
    // If head node itself holds the key or multiple occurrences of key
    while (temp != NULL && temp->sockid == sock)
    {
        *head = temp->next;   // Changed head
        free(temp);               // free old head
        temp = *head;         // Change Temp
    }
 
    // Delete occurrences other than head
    while (temp != NULL)
    {
        // Search for the key to be deleted, keep track of the
        // previous node as we need to change 'prev->next'
        while (temp != NULL && temp->sockid != sock)
        {
            prev = temp;
            temp = temp->next;
        }
 
        // If key was not present in linked list
        if (temp == NULL) return;
 
        // Unlink the node from linked list
        prev->next = temp->next;
 
        free(temp);  // Free memory
 
        //Update Temp for next iteration of outer loop
        temp = prev->next;
    }
}
