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
	int64_t count;
	int64_t owner;
};

typedef struct bucket_lock bucket_lock;

class SyncHash : private Hash {
	public:
	static const size_t DEFAULT_SIZE = Hash::DEFAULT_SIZE;
	SyncHash(): SyncHash(Hash::DEFAULT_SIZE) {}
	SyncHash(size_t size, uint16_t recur = DEFAULT_RECUR, char* _logdir = DEFAULT_LOG_DIR);
	template<class dataType>
	int8_t insert(uint8_t*, dataType, int64_t);
	int8_t remove(uint8_t*, int64_t);
	int8_t lookup(uint8_t*, uint8_t*&, int64_t);
	int8_t lookup(uint8_t*, int64_t&, int64_t);
	int8_t rlookup(uint8_t*, int64_t&, int64_t);
	void clear(int64_t);
	int8_t dump(const char*, int64_t);
	int8_t load(const char*, int64_t);
	int8_t acquire(uint8_t*, int64_t);
	int8_t acquire(uint8_t**, size_t, int64_t);
	int8_t release(uint8_t*, int64_t);
	void release(uint8_t**, size_t, int64_t);
	size_t size() { return tblSize; }
	private:
	void acquire(uint32_t, int64_t);
	void acquire_all(int64_t);
	void release(uint32_t);
	void release_all();
	bucket_lock* locks;
};

/**
 * Insert
 * @param key: The key to be inserted into the hashtable
 * @param value: value to insert into the node
 * @param ident: Thread identifier 
 *
 * Hashes the key and uses the hash value to insert
 * a new node into the table. Replaces an already
 * existing Node
 **/
template<class dataType>
int8_t SyncHash::insert(uint8_t *key, dataType value, int64_t ident)
{
	int32_t hash = genHash(key);
	if (hash == -1) {
		return -1;
	}
	acquire(hash, ident);
	if (Hash::insert(key, value) == -1) {
		return -1;
	}
	release(hash);
	return 0;
}

#endif