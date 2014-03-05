/*
 * File: multilookup.c
 * Author: Kristina Entzel
 * Project: CSCI 3753 Programming Assignment 2
 */
//Add mnutex destroy
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include "util.h"
#include "queue.h"

#define MINARGS 3
#define USAGE "<inputFilePath> <outputFilePath>"
#define SBUFSIZE 1025
#define INPUTFS "%1024s"
#

//Global values
queue* requesterQ;
int SearchComplete = 0;
FILE* outputfp;
pthread_mutex_t Q_lock;
pthread_mutex_t Out_lock;

//Read input file function
void* ReqThreads(void* Inputfile) 
{
	char* hostname;
	hostname = malloc(SBUFSIZE*sizeof(char)); //use malloc to allocate memory
	FILE* fp = fopen(Inputfile, "r");
	
	//Read File and Process, list of text, url hostnames
	while (fscanf(fp, INPUTFS, hostname) > 0) {
		
		//If queue is full, wait and sleep
		while(queue_is_full(requesterQ)) { //busy wait
			usleep((rand()% 100)+1);
			continue;   
			} 
		
			pthread_mutex_lock(&Q_lock); //lock so only one thread can access queue
			queue_push(requesterQ, hostname); //push the hostname onto the queue
			pthread_mutex_unlock(&Q_lock); //unlock when done pushing an item

			hostname = malloc(SBUFSIZE*sizeof(char));
	}
	free(hostname); //free the space allocated with malloc
	fclose(fp); //close input file when done
	return 0;
}

void* ResThreads(void* empty)
{
	char* outhostname; //hostname you pop off the queue
	char firstipstring[INET6_ADDRSTRLEN];
	
	//while the search is incomplete or the queue is not empty
	while (!queue_is_empty(requesterQ) || !SearchComplete)
	{
		//if queue is empty wait
		while (queue_is_empty(requesterQ)){
			if(SearchComplete)
			{
				free(outhostname);
				return 0;
			}
		}
		//if there is an item in the queue, pop 
		pthread_mutex_lock(&Q_lock);//lock to have exclusion
		outhostname = queue_pop(requesterQ); //hostname 
		pthread_mutex_unlock(&Q_lock);
		
		//Lookup hostname and get IP string
		if(dnslookup(outhostname, firstipstring, sizeof(firstipstring)) == UTIL_FAILURE)
		{
			fprintf(stderr, "dnslookup error: %s\n", outhostname);
			strncpy(firstipstring, "", sizeof(firstipstring));
		}
		//Write to output file
		pthread_mutex_lock( &Out_lock ); //Lock the outputfile so you only put one thing at a time in it
		fprintf(outputfp, "%s,%s\n", outhostname, firstipstring);
		free(outhostname);
		pthread_mutex_unlock( &Out_lock );
	}
	return 0;
}

int main(int argc, char* argv[]) {
	//Local variables
	int i;
	int numfiles = (argc - 2);
	int numCores = sysconf(_SC_NPROCESSORS_ONLN);
	if (numCores < 2){
		numCores = 2;
	}
	pthread_t req_threads[numfiles];
	pthread_t* res_threads; // Pointer threads because determined with malloc, at runtime.

	res_threads = malloc (numCores * sizeof(pthread_t));
	int rc; // race condition
	

	//Check Arguments
    if(argc < MINARGS)
    {
	fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
	fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
	return EXIT_FAILURE;
    }
    
    //initialize data structures: queue
	requesterQ = malloc(sizeof(queue));
    if(queue_init(requesterQ,50) == QUEUE_FAILURE ) return EXIT_FAILURE;

    //Open Output File */
    outputfp = fopen(argv[(argc-1)], "w");
    if(!outputfp){
	perror("Error Opening Output File");
	return EXIT_FAILURE;
    }
	//Loop Through Input Files
    for (i = 0; i < numfiles; i++)
    {
		//check that the File is valid
		FILE* current = fopen(argv[i], "r");
		if(!current){
	    fprintf(stderr, "Error Opening Input File \n");
	    return EXIT_FAILURE;
		}
		
		//Create the requester threads	
    	rc = pthread_create(&(req_threads[i]), NULL, ReqThreads, argv[i+1]);
    	if (rc)
    	{
    		printf("Error: Return code from pthread_create() is %d\n", rc);
    		exit(EXIT_FAILURE);
    	}
    }
	//Create the resolver threads
    for (i = 0; i < numfiles; i++)
    {
    	rc = pthread_create(&(res_threads[i]), NULL, ResThreads, argv[i+1]);
    	if (rc)
    	{
    		printf("Error: Return code from pthread_create() is %d\n", rc);
    		exit(EXIT_FAILURE);
    	}
    }

    //Join requester threads
    for (i = 0; i < numfiles; i++)
    {
    	pthread_join(req_threads[i], NULL);
    }
    SearchComplete = 1;

   //Join the requester threads and resolver threads, making them program
	// sit and wait for the subprocess threads to finish enqueuing and dequeing
	//searching, and copying into the output file
    for (i = 0; i < numCores; i++)
    {
    	pthread_join(res_threads[i], NULL);
    }
	pthread_mutex_destroy(&Q_lock);
    pthread_mutex_destroy(&Out_lock);
    
    //Close Output File 
    fclose(outputfp);
    queue_cleanup(requesterQ); //Clear queue
    free(requesterQ);
    free(res_threads); // Free memory allocation

    return 0;
} 

