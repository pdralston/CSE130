#ifndef SERVER_STRUCTS
#define SERVER_STRUCTS
 /**
 * structs
 *
 * Supporting structures for the rpcserver
 *
 * @author Perry David Ralston Jr.
 * @date 11/18/2020
 **/

#include <inttypes.h>
#include <pthread.h>
#include <semaphore.h>

#include "sync_hash.h"


struct resp_header {
	uint8_t req_ident[4];
	uint16_t op = 0;
	uint8_t err_code = 0;
};

struct thread {
	sem_t* mainMutex;
	sem_t mutex;
	SyncHash* hTable; 
	int cl;
	int64_t ident;
};

#endif
