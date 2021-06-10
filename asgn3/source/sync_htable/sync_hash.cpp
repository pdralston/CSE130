/**
 * Synchronized Hash Table source file
 * @author Perry David Ralston Jr.
 * @date 11/20/2020
 */

#include <algorithm>
#include <err.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "sync_hash.h"

SyncHash::SyncHash(size_t size, uint16_t recur, char* _logdir) : Hash(size, recur, _logdir) {
	locks = (bucket_lock*)calloc(tblSize, sizeof(bucket_lock));
	for (size_t i = 0; i < tblSize; ++i) {
		bucket_lock current = locks[i];
		current.count = 0;
		current.owner = NO_PARENT;
		if (0 != sem_init(&current.mutex, 0, 0)) err(2,"sem_init synch_hash.locks[]");
	}
}

/**
 * Remove
 * @param key: The key to be removed from the hashtable
 * @param ident: Thread identifier
 * @return: 0 if remove is successful, -1 otherwise. Sets errno appropriately
 * Hashes the key and seeks for the matching node. Deletes the node and resolves
 * any dangling pointers that might result from the removal.
 **/
int8_t SyncHash::remove(uint8_t *key, int64_t ident)
{
	int32_t hash = genHash(key);
	if (hash == -1) {
		return -1;
	}
	acquire(hash, ident);
	if (Hash::remove(key) == -1) {
		return -1;
	}
	release(hash);
	return 0;
}

/**
 * Lookup
 * @param key: The key to be inserted into the hashtable
 * @param value: Signed integer value reference to assign the value to
 * @param ident: Thread identifier
 * @return: 0 if lookup is successful, -1 otherwise. Sets errno appropriately
 *
 * Hashes the key and uses the hash value to find
 * the corresponding node in the table. If the node is found
 * value is set to the value of the node and 0 is returned. -1 is returned
 * otherwise.
 **/
int8_t SyncHash::lookup(uint8_t *key, int64_t &value, int64_t ident)
{
	int32_t hash = genHash(key);
	if (hash == -1) {
		errno = EINVAL;
		return -1;
	}
	acquire(hash, ident);
	int8_t ret_val = Hash::lookup(key, value);
	release(hash);
	return ret_val;
}

int8_t SyncHash::lookup(uint8_t *key, uint8_t*& value, int64_t ident)
{
	int32_t hash = genHash(key);
	if (hash == -1) {
		errno = EINVAL;
		return -1;
	}
	acquire(hash, ident);
	int8_t ret_val = Hash::lookup(key, value);
	release(hash);
	return ret_val;
}

int8_t SyncHash::rlookup(uint8_t *key, int64_t &value, int64_t ident) {
	acquire_all(ident);
	int8_t ret_val = Hash::rlookup(key, value, get_recur_amt());
	release_all();
	return ret_val;
}

/**
 * Clear
 * clears all records from the hash table, freeing the associated memory
 * @param ident: Thread identifier
 **/
void SyncHash::clear(int64_t ident)
{
	acquire_all(ident);
	Hash::clear();
	release_all();
}

/**
 * Dump
 * @param filename: File to save the contents of the hash table to
 * @param ident: Thread identifier
 * @return: 0 if dump is successful, -1 otherwise. Sets errno appropriately
 *
 * Parses the hash table and prints each value key pair, seperated by '=',
 * on a new line. This function has the same potential failures as open(2)
 * and write(2).
 **/
int8_t SyncHash::dump(const char *filename, int64_t ident)
{
	acquire_all(ident);	
	int8_t ret_val = Hash::dump(filename);
	release_all();
	return ret_val;
}

/**
 * Load
 * @param filename: File to load entries from
 * @param ident: Thread identifier
 * @return: 0 if load is successful, -1 otherwise. Sets errno appropriately
 *
 * Parses the file and inserts '=' seperated key-value pairs into the hash table.
 * This function has the same potential failures as open(2) and read(2).
 **/
int8_t SyncHash::load(const char *filename, int64_t ident)
{
	acquire_all(ident);	
	int8_t ret_val = Hash::load(filename);
	release_all();
	return ret_val;
}

/**
 * acquire
 * @param key: Key to acquire lock on
 * @param ident: Thread identifier
 * @details Attempt to acquire the lock of the list associated with
 * the key. Checks if the lock is already claimed by this thread  
 **/
int8_t SyncHash::acquire(uint8_t * key, int64_t ident) {
	int32_t hash = genHash(key);
	if (hash == -1) {
		return -1;
	}
	acquire(hash, ident);
	return 0;
}

/**
 * acquire
 * @param keylist: Keys to acquire lock on
 * @param size: number of keys to acquire
 * @param ident: Thread identifier
 * @details Attempt to acquire the locks of the lists associated with
 * the keys in index order. Checks if the lock is already claimed by this thread  
 **/
int8_t SyncHash::acquire(uint8_t** keylist, size_t size, int64_t ident) {
	int32_t* hashlist = (int32_t*)calloc(size, sizeof(int32_t));
	for (size_t i = 0; i < size; ++i) {
		if (keylist[i] != nullptr) {
			hashlist[i] = genHash(keylist[i]);
		}
	}
	std::sort(hashlist, hashlist + size);
	if (hashlist[0] == -1) {
		return -1;
	} 
	for (size_t i = 0; i < size; ++i) {
		acquire(hashlist[i], ident);
	}
	free(hashlist);
	return 0;	 
}

void SyncHash::acquire(uint32_t hash, int64_t ident) {
	if (tblSize <= hash) {//this is just a sanity check, not possible to be true
		return;
	}
	if (ident != locks[hash].owner) {
		if (0 != sem_wait(&locks[hash].mutex)) err(2,"sem_wait in sync_hash");
		locks[hash].owner = ident;
	}
	++locks[hash].count;
}

void SyncHash::acquire_all(int64_t ident) {
	for(size_t i = 0; i < tblSize; ++i) {
		acquire(i, ident);
	}
}

/**
 * release
 * @param key: Key to release lock on
 * @param ident: Thread identifier
 * reset the ownership of the lock to NO_PARENT
 * and signal on the mutex if this thread owns the lock
 * @return: 0 if load is successful, -1 otherwise. Sets errno appropriately
 **/
int8_t SyncHash::release(uint8_t* key, int64_t ident) {
	int32_t hash = genHash(key);
	if (hash == -1) {
		return -1;
	}
	if (locks[hash].owner != ident) {
		errno = EINVAL;
		return -1;
	}
	release(hash);
	return 0;
}

/**
 * release
 * @param keylist: Keys to release lock on
 * @param size: number of keys to release locks on
 * @param ident: Thread identifier
 * resets the ownership of each lock to NO_PARENT
 * and signal on the mutex if the lock exists and this thread owns the lock
 **/
void SyncHash::release(uint8_t** keylist, size_t size, int64_t ident){
	int32_t* hashlist = (int32_t*)calloc(size, sizeof(int32_t));
	for (size_t i = 0; i < size; ++i) {
		if(keylist[i] != nullptr) {
			hashlist[i] = genHash(keylist[i]);
		}
	}
	for (size_t i = 0; i < size; ++i) {
		if (hashlist[i] != -1 && locks[hashlist[i]].owner == ident) {
			release(hashlist[i]);
		}
	}
}

void SyncHash::release(uint32_t hash) {
	if (locks[hash].count > 0) { 
		--locks[hash].count;
		if (locks[hash].count == 0) {
			if (0 != sem_post(&locks[hash].mutex)) err(2,"sem_post in sync_hash");
			locks[hash].owner = NO_PARENT;
		}
	}
}

void SyncHash::release_all() {
	for(size_t i = 0; i < tblSize; ++i) {
		release(i);
	}
}
