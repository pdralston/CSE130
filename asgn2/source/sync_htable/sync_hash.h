/**
 * Synchronized Hash Table Header File
 * @author Perry David Ralston Jr.
 * @date 11/20/2020
 */
#ifndef SYNCH_HASHH
#define SYNCH_HASHH

#include <inttypes.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "hash.h" 

static const int8_t NO_PARENT = -1;

struct bucket_lock {
	sem_t mutex;
	int64_t owner;
};

typedef struct bucket_lock bucket_lock;

class SyncHash : public Hash {
	public:
	SyncHash(size_t size);
	void insert(uint8_t*, int64_t, int64_t);
	int8_t remove(uint8_t*, int64_t);
	int8_t lookup(uint8_t*, int64_t&, int64_t);
	void clear(int64_t);
	int8_t dump(const char*, int64_t);
	int8_t load(const char*, int64_t);
	void acquire(uint8_t*, int64_t);
	void release(uint8_t*);
	private:
	void acquire(uint32_t, int64_t);
	void acquire_all(int64_t);
	void release(uint32_t hash);
	void release_all();
	bucket_lock* locks;
};

#endif