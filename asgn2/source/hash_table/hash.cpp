/**
 * Hash Table Source File
 * Originally written 3/2017
 * @author Perry David Ralston Jr.
 * modified:
 * @date 11/12/2020
 */

#include <cmath>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "hash.h"

Node::Node(uint8_t* nKey, int64_t nEntry) {
	data = nEntry; 
	next = nullptr;
	key = (uint8_t*) calloc(MAX_KEY_LENGTH, sizeof(uint8_t));
	strcpy((char*)key, (char *)nKey);
}

Node::~Node() {
	free(key);
	key = nullptr;
}

Hash::Hash(size_t size) {
	tblSize = size; 
	hTable = (Node **)calloc(tblSize, sizeof(Node *));
}

/**
 * Insert
 * @param key: The key to be inserted into the hashtable
 * @param value: Signed integer value associated to key
 *
 * Hashes the key and uses the hash value to insert
 * a new node into the table. Replaces an already
 * existing Node
 **/
void Hash::insert(uint8_t *key, int64_t value)
{
	uint32_t hash = genHash(key);
	Node *new_node = new Node(key, value);
	Node *current = hTable[hash];
	if(current == nullptr) {
		hTable[hash] = new_node;
		return;
	}
	while(current != nullptr) {
		if(strcmp((char *)current->key, (char *)key) == 0) {
			Node *temp = current;
			current = new_node;
			current->next = temp->next;
			delete(temp);
			temp = nullptr;
			return;
		}
		if(current->next == nullptr) {
			current->next = new_node;
			return;
		}
		current = current->next;
	}
}

/**
 * Remove
 * @param key: The key to be removed from the hashtable
 * @return: 0 if remove is successful, -1 otherwise. Sets errno appropriately
 * Hashes the key and seeks for the matching node. Deletes the node and resolves
 * any dangling pointers that might result from the removal.
 **/
int8_t Hash::remove(uint8_t *key)
{
	uint32_t hash = genHash(key);
	Node* current = hTable[hash];
	if (current != nullptr && strcmp((char*)current->key, (char*)key) == 0) {
		hTable[hash] = current->next;
		return 0;		
	}
	Node* follower = current; 
	while (current != nullptr) {
		if (strcmp((char*)current->key, (char*)key) == 0) {
			follower->next = current->next;
			return 0;
		}
		follower = current;
		current = current->next; 
	}
	//key not found in hashtable
	errno = EINVAL;
	return -1;
}

/**
 * Lookup
 * @param key: The key to be inserted into the hashtable
 * @param value: Signed integer value reference to assign the value to
 * @return: 0 if lookup is successful, -1 otherwise. Sets errno appropriately
 *
 * Hashes the key and uses the hash value to find
 * the corresponding node in the table. If the node is found
 * value is set to the value of the node and 0 is returned. -1 is returned
 * otherwise.
 **/
int8_t Hash::lookup(uint8_t *key, int64_t &value)
{
	uint32_t hash = genHash(key);
	Node* current = hTable[hash];
	while (current != nullptr) {
		if (strcmp((char*)current->key, (char*)key) == 0) {
			value = current->data;
			return 0;
		}
		current = current->next; 
	}
	//key not found in hashtable
	errno = EINVAL;
	return -1;
}

/**
 * Clear
 * clears all records from the hash table, freeing the associated memory
 **/
void Hash::clear()
{
	for (size_t i = 0; i < tblSize; ++i) {
		Node* current = hTable[i];
		while (current != nullptr) {
			Node* temp = current;
			current = current->next;
			delete(temp);
		}
	}
}

/**
 * Dump
 * @param filename: File to save the contents of the hash table to
 * @return: 0 if dump is successful, -1 otherwise. Sets errno appropriately
 *
 * Parses the hash table and prints each value key pair, seperated by '=',
 * on a new line. This function has the same potential failures as open(2)
 * and write(2).
 **/
int8_t Hash::dump(const char *filename)
{
	int fd = open(filename, O_WRONLY);
	if (fd == -1) { return -1; }
	const size_t buff_size = DEFAULT_SIZE + sizeof(int64_t) + 3; //32bit var name + '=' + int64 + '\n' + '\0' 
	char buffer[buff_size];
	for (size_t i = 0; i < tblSize; ++i) {
		Node* current = hTable[i]; 
		while ( current != nullptr) {
			int print_size = snprintf(buffer, buff_size, "%s=%ld\n", current->key, current->data);
			if (print_size < 0) { return -1; }
			if (write(fd, buffer, print_size) == -1) { return -1; }
			current = current->next;
		}
	}
	return 0;
}

/**
 * Load
 * @param filename: File to load entries from
 * @return: 0 if load is successful, -1 otherwise. Sets errno appropriately
 *
 * Parses the file and inserts '=' seperated key-value pairs into the hash table.
 * This function has the same potential failures as open(2) and read(2).
 **/
int8_t Hash::load(const char *filename)
{
	FILE* fp = fopen(filename, "r");
	if (fp == nullptr) {return -1;}
	char* line;
	size_t size = 0;
	ssize_t read = 0;
	uint8_t key[DEFAULT_SIZE];
	int64_t value;
	while ((read = getline(&line, &size, fp)) != -1) {
		if((size_t)(strstr(line, "=") - line) > DEFAULT_SIZE) { 
			errno = EINVAL;
			return -1;
		}
		sscanf(line, "%[^=]=%ld", key, &value);
		insert(key, value);
	}
	return 0;
}

uint32_t Hash::genHash(uint8_t *key)
{
	size_t size = strlen((char *)key);
	int32_t key_value = 0;
	for(size_t i = 0; i < size; ++i) {
		key_value += key[i];
	}
	return floor(tblSize * ((key_value * HASHCONST) - floor(key_value * HASHCONST)));
}
