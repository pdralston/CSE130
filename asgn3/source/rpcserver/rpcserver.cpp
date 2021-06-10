/**
 * rpcserver
 *
 * Multi-threaded rpc server
 *
 * @author Perry David Ralston Jr.
 * @date 10/19/2020
 **/

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "sync_hash.h"
#include "structs.h"
#include "process.h"

const int16_t MAX_HOSTNAME_SIZE = 1000;
const int16_t MIN_PORT_VAL = 1024;

typedef struct thread Thread;

void* start(void* arg);
int getWorker(Thread*, const int&);

int main(int argc, char *argv[])
{
	//main variables
	char host_name[MAX_HOSTNAME_SIZE];
	uint8_t* data_dir = (uint8_t*)calloc(MAX_HOSTNAME_SIZE, sizeof(uint8_t));
	strcpy((char*)data_dir, "data");
	uint16_t port = 0;
	uint16_t recur = 50;
	int num_threads = 4, cl = 0, htable_size = 32, opt, worker;
	SyncHash* hTable;
	Thread* threads;
	pthread_t dummy_addr;
	sem_t mainMutex;

	//handle command line args
	while ((opt = getopt(argc, argv, "N:H:I:d:")) != -1) {
		switch (opt) {
		case 'N':
			num_threads = atoi(optarg);
			break;
		case 'H':
			htable_size = atoi(optarg);
			break;
		case 'I':
			recur = atoi(optarg);
			break;
		case 'd':
			if(MAX_HOSTNAME_SIZE < strlen(optarg)) {
				err(EXIT_FAILURE, "Invalid Pathname\n");
			}
			strcpy((char*)data_dir, (char*)optarg);
			break;
		default: /* '?' */
			break;
		}
	}
	if (argv[optind] == nullptr) {
		errx(EXIT_FAILURE, "Usage: ./rpcserver <host_name>:<port>");
	}
	// hostname is too large
	if(strstr(argv[optind], ":") - argv[optind] > MAX_HOSTNAME_SIZE) {
		errx(EXIT_FAILURE, "hostname too large");
	}
	if(sscanf(argv[optind], "%[^:]:%hu", host_name, &port) < 2) {
		errx(EXIT_FAILURE, "Argument format <host_name>:<port>");
	}
	if(port < MIN_PORT_VAL) {
		errx(EXIT_FAILURE, "Port must be > %d", MIN_PORT_VAL);
	}

	hTable = new SyncHash(htable_size, recur, (char*) data_dir);

	//init threads
	//some code below sourced from https://piazza.com/class/kex6x7ets2p35c?cid=291 11/18/2020
	threads = (Thread*)calloc(num_threads, sizeof(Thread));
	
	if (0 != sem_init(&mainMutex, 0, 0)) err(2,"sem_init mainMutex");

	for (int i = 0; i < num_threads; ++i) {
		Thread* thread = &threads[i];
		thread->cl = 0;
		thread->mainMutex = &mainMutex;
		thread->hTable = hTable;
		thread->ident = i;
        if (0 != pthread_create(&dummy_addr,0,start,thread)) err(2,"pthread_create");
	}

	//init network
	/* Sourced from:
	 * https://canvas.ucsc.edu/courses/36179/pages/setting-up-sockets-for-basic-client-slash-server-stream-communication
	 */
	struct hostent *hent = gethostbyname(host_name);
	struct sockaddr_in addr;
	memcpy(&addr.sin_addr.s_addr, hent->h_addr, hent->h_length);
	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	int enable = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
	if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
		err(EXIT_FAILURE, "%s\n", strerror(errno));
	listen(sock, 128);
	while(true) {
		cl = accept(sock, NULL, NULL);
		if(cl == -1) {
			err(EXIT_FAILURE, "%s", strerror(errno));
		}
		while((worker = getWorker(threads, num_threads))==-1) {
			if (0 != sem_wait(&mainMutex)) err(2,"sem_wait in main");
		}
		threads[worker].cl = cl;
		if (0 != sem_post(&(threads[worker].mutex))) err(2,"sem_post for thread"); 
	}
	return 0;
}


void* start(void* arg) {
	Thread* self = (Thread*) arg;
	while (true) { 
		while(self->cl == 0) { 
			if (0 != sem_wait(&self->mutex)) { 
				err(2, "sem_wait in thread"); 
			} 
		}
		process(self);
		self->cl = 0;
		if (0 != sem_post(self->mainMutex)) err(2, "sem_post of main in thread");
	}
}

int getWorker(Thread* threads, const int& num_threads) {
	for (int i = 0; i < num_threads; ++i) {
		if (threads[i].cl == 0) {
			return i;
		}
	}
	return -1;
}