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
 * verifying solutions and getting solutions from
 * work messages.
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "uint256.h"
#include "sha256.h"

#define BASE_2 2
#define CONCAT_SIZE 40
#define MAX_MESSAGE_LENGTH 256
#define UINT256_BLOCKSIZE 32
#define HEX_BASE 16


/*Function prototypes*/
int verify_solution(char *token);
void itoB(uint32_t x, BYTE uint256b[UINT256_BLOCKSIZE]);
void convertBytes(void *p, size_t len, BYTE uint256b[UINT256_BLOCKSIZE]);
uint32_t getAlpha(char *diff);
uint32_t getBeta(char *diff);
void getX(BYTE x[CONCAT_SIZE], char cat[MAX_MESSAGE_LENGTH]);
void getTarget(BYTE target[UINT256_BLOCKSIZE], uint32_t a, uint32_t b);
int solve(char* token, char *solution_message);

/*Verifies a solution message, returns 1 if valid solution -1 otherwise*/
int verify_solution(char *token) {
	char buffer[MAX_MESSAGE_LENGTH], diff[MAX_MESSAGE_LENGTH], seed[MAX_MESSAGE_LENGTH], nonce[MAX_MESSAGE_LENGTH];
	long ret;
	uint32_t a,b;

	BYTE x[CONCAT_SIZE];
	BYTE target[UINT256_BLOCKSIZE];
	SHA256_CTX ctx;
	BYTE hx[SHA256_BLOCK_SIZE];
	BYTE y[SHA256_BLOCK_SIZE];
	
	strcpy(buffer, token);
	
	if(sscanf(buffer, "SOLN %s %s %s", diff, seed, nonce) != 3){
		return -1;	
	}
	
	if(strlen(diff) != 8){
		return -1;
	}
	
	if(strlen(seed) != 64){
		return -1;
	}
	
	if(strlen(nonce) != 16){
		return -1;
	}
	
	
	ret =  strtoul(diff, NULL, UINT256_BLOCKSIZE);
	
	strcat(seed, nonce);
	
	getX(x, seed);
	
	a = getAlpha(diff);
	b = getBeta(diff);
	
	getTarget(target, a, b);
	
	/*SHA Stuff*/
	sha256_init(&ctx);
	sha256_update(&ctx, x, CONCAT_SIZE);
	sha256_final(&ctx, hx);
	//printf("H(x):");
	//print_uint256(hx);
	
	//printf("H(H(x)):");
	sha256_init(&ctx);
	sha256_update(&ctx, hx, UINT256_BLOCKSIZE);
	sha256_final(&ctx, y);
	//print_uint256(y);
	
	if(sha256_compare(y, target) == -1){
		return 1;
	}
	else{
		return -1;
	}
}

/* Code segment taken from 
 * https://stackoverflow.com/questions/12378615/how-to-print-the-byte-representations-of-a-short-int-c-object
 * and modified for use to convert and store into unit256 implemented using a BYTE array
*/
void convertBytes(void *p, size_t len, BYTE uint256b[UINT256_BLOCKSIZE]) {
	int j;
    size_t i;
	j = 31;
    for (i = 0; i < len; ++i){
		uint256b[j] = ((unsigned char*)p)[i];
		j --;
	}
}

void itoB(uint32_t x, BYTE uint256b[UINT256_BLOCKSIZE]) {
    convertBytes(&x, sizeof(x), uint256b);
}

/*Gets the X value in byte form from the concatenation of seed and nonce*/
void getX(BYTE x[CONCAT_SIZE], char cat[MAX_MESSAGE_LENGTH]) {
	int byte_count, i;
	char byte_buffer[5];
	
	byte_count = 0;
	byte_buffer [0] = '0';
	byte_buffer [1] = 'x';
	byte_buffer [4] = '\0';
	
	uint256_init (x);
	for(i=0; i<strlen(cat); i+=2){
		byte_buffer[2] = cat[i];
		byte_buffer[3] = cat[i+1];
		/*printf("%s\n", byte_buffer );*/
		x[byte_count] = (BYTE)strtol(byte_buffer,NULL,HEX_BASE);
		/*printf("%c\n", x[byte_count]);*/
		byte_count ++;
	}
}

/*Gets alpha from the difficulty through bit shifting*/
uint32_t getAlpha(char *diff){
	uint32_t a;
	a = strtol(diff, NULL, HEX_BASE);
	a = a>>24;
	return a;
}

/*Gets beta from the difficulty through bit shifting*/
uint32_t getBeta(char *diff) {
	uint32_t b;
	b = strtol(diff, NULL, HEX_BASE);
	b = b<<8;
	b = b>>8;
	return b;
}

/*Calculates the target given the alpha and beta*/
void getTarget(BYTE target[UINT256_BLOCKSIZE], uint32_t a, uint32_t b) {
	uint32_t exponent;
	BYTE uint256b[UINT256_BLOCKSIZE], uint256_base[UINT256_BLOCKSIZE], uint256c[UINT256_BLOCKSIZE];
	
	uint256_init (uint256b);
	uint256_init (uint256c);
	uint256_init (uint256_base);
	uint256_init (target);
	
	/*Gets beta in byte representation*/
	itoB(b, uint256b);
	
	itoB(BASE_2, uint256_base);
	
	exponent = 8*(a-3);
	
	uint256_exp (uint256c, uint256_base, exponent);
	
	uint256_mul (target, uint256c, uint256b);
	//printf("Target:");
	//print_uint256(target);
}

/*Solves a solution message, copies the solution message into the second argument*/
int solve(char *token, char *solution_message) {
	int i;
	long unsigned int nonce;
	char buffer[MAX_MESSAGE_LENGTH], diff[MAX_MESSAGE_LENGTH], seed[MAX_MESSAGE_LENGTH], workers[MAX_MESSAGE_LENGTH],
	solution[MAX_MESSAGE_LENGTH], start[MAX_MESSAGE_LENGTH], checknonce[MAX_MESSAGE_LENGTH];
	
	if(sscanf(token, "WORK %s %s %lx %s\r\n", diff, seed, &nonce, workers) != 4){
		return -1;
	}

	
	sscanf(token, "WORK %s %s %s %s\r\n", diff, seed, checknonce, workers);
	
	if(strlen(diff) != 8){
		return -1;
	}
	
	if(strlen(seed) != 64){
		return -1;
	}
	
	if(strlen(checknonce) != 16){
		return -1;
	}
	
	
	sprintf(start, "%lx", nonce);

	while(1) {
		bzero(solution, MAX_MESSAGE_LENGTH);
		snprintf(solution, MAX_MESSAGE_LENGTH, "SOLN %s %s %s", diff, seed, start);
		if(verify_solution(solution) == -1) {
			nonce++;
			bzero(start, MAX_MESSAGE_LENGTH);
			sprintf(start, "%lx", nonce);
		}
		else {
			sprintf(solution_message, "%s\r\n", solution);
			return 1;
		}
	}
}