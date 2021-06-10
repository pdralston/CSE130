/**
 * Synchronized Hash Table source file
 * @author Perry David Ralston Jr.
 * @date 11/20/2020
 */

#include <err.h>
#include <errno.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "sync_hash.h"

SyncHash::SyncHash(size_t size) : Hash(size) {
	locks = (bucket_lock*)calloc(tblSize, sizeof(bucket_lock));
	for (size_t i = 0; i < tblSize; ++i) {
		bucket_lock current = locks[i];
		current.owner = -1;
		if (0 != sem_init(&current.mutex, 0, 0)) err(2,"sem_init synch_hash.locks[]");
	}
}

/**
 * Insert
 * @param key: The key to be inserted into the hashtable
 * @param value: Signed integer value associated to key
 * @param ident: Thread identifier 
 *
 * Hashes the key and uses the hash value to insert
 * a new node into the table. Replaces an already
 * existing Node
 **/
void SyncHash::insert(uint8_t *key, int64_t value, int64_t ident)
{
	acquire(key, ident);
	Hash::insert(key, value);
	release(key);
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
	acquire(key, ident);
	int8_t ret_val = Hash::remove(key);
	release(key);
	return ret_val;
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
	acquire(key, ident);
	int8_t ret_val = Hash::lookup(key, value);
	release(key);
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
void SyncHash::acquire(uint8_t * key, int64_t ident) {
	uint32_t hash = genHash(key);
	acquire(hash, ident);
}

void SyncHash::acquire(uint32_t hash, int64_t ident) {
	if (ident == locks[hash].owner) {
		return;
	}
	if (0 != sem_wait(&locks[hash].mutex)) err(2,"sem_wait in sync_hash");
	locks[hash].owner = ident;
}

void SyncHash::acquire_all(int64_t ident) {
	for(size_t i = 0; i < tblSize; ++i) {
		acquire(i, ident);
	}
}

/**
 * release
 * @param key: Key to acquire lock on
 * reset the ownership of the lock to NO_PARENT
 * and signal on the mutex 
 **/
void SyncHash::release(uint8_t * key) {
	uint32_t hash = genHash(key);
	release(hash);
}

void SyncHash::release(uint32_t hash) {
	if (0 != sem_post(&locks[hash].mutex)) err(2,"sem_post in sync_hash");
	locks[hash].owner = NO_PARENT;
}

void SyncHash::release_all() {
	for(size_t i = 0; i < tblSize; ++i) {
		release(i);
	}
}
