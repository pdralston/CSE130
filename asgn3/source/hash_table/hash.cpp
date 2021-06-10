/**
 * Hash Table Source File
 * Originally written 3/2017
 * @author Perry David Ralston Jr.
 * modified:
 * @date 11/12/2020
 */

#include <cmath>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <regex>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "hash.h"

Node::~Node() {
	free(key);
	key = nullptr;
	if (!isNum) {
		free(data.var_val);
	}
}

Hash::Hash(size_t size, uint16_t recur, char* _logdir) {
	tblSize = size > 0 ? size : DEFAULT_SIZE; 
	hTable = (Node **)calloc(tblSize, sizeof(Node *));
	recur_amt = 0 <= recur ? recur : DEFAULT_RECUR;
	log_dir = open(_logdir, O_DIRECTORY);
	if (log_dir == -1) {
		err(EXIT_FAILURE, "%s\n", strerror(errno));
	}
	int fd = openat(log_dir, LOGFILE, O_RDONLY);
	init_load = true;
	if (fd > 0) {
		logfile = fdopen(fd, "r");
		if (logfile != nullptr) {
			load(logfile);
		}
	}
	init_load = false;
	fd = openat(log_dir, LOGFILE, O_RDWR|O_CREAT|O_TRUNC, S_IRWXU);
	if (fd == -1) {
		err(EXIT_FAILURE, "%s\n", strerror(errno));
	}
	logfile = fdopen(fd, "w+");
	if (logfile == nullptr) {
		err(EXIT_FAILURE, "%s\n", strerror(errno));
	}
	load_dump();
}

Hash::~Hash() {
	clear();
	fclose(logfile);
	free(hTable);
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
	int32_t hash = genHash(key);
	if (hash == -1 || remove(hash, key) == -1) {
		errno = ENOENT;
		return -1;
	}
	if (!init_load) {
		fprintf(logfile,"~%s=\n",key);
		sync_log();
	}
	return 0;
}

int8_t Hash::remove(int32_t hash, uint8_t* key)
{
	Node* current = hTable[hash];
	if (current != nullptr && strcmp((char*)current->key, (char*)key) == 0) {
		hTable[hash] = current->next;
		delete(current);
		return 0;		
	}
	Node* follower = current; 
	while (current != nullptr) {
		if (strcmp((char*)current->key, (char*)key) == 0) {
			follower->next = current->next;
			delete(current);
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

int8_t Hash::lookup(uint8_t *key, int64_t& value)
{
	Node* retrvd = nullptr;
	int32_t hash = genHash(key);
	if (hash == -1) {
		errno = ENOENT;
		return -1;
	}
	if (lookup(hash, key, retrvd) == 0) {
		if (retrvd->isNum) {
			value = retrvd->data.num_val;
			return 0;
		}
		errno = EFAULT;
	}
	return -1;
}

int8_t Hash::lookup(uint8_t *key, uint8_t* &value)
{
	Node* retrvd = nullptr;
	int32_t hash = genHash(key);
	if (hash == -1) {
		errno = ENOENT;
		return -1;
	}
	if (lookup(hash, key, retrvd) == 0) {
		if (!retrvd->isNum) {
			strcpy((char*)value, (char*)retrvd->data.var_val);
			return 0;
		}
		errno = EFAULT;
	}
	return -1;
}

int8_t Hash::lookup(int32_t hash, uint8_t* key, Node*& value)
{
	Node* current = hTable[hash];
	while (current != nullptr) {
		if (strcmp((char*)current->key, (char*)key) == 0) {
			value = current;
			return 0;
		}
		current = current->next; 
	}
	//key not found in hashtable
	errno = ENOENT;
	return -1;
}

int8_t Hash::rlookup(uint8_t* key, int64_t& data, uint16_t recur) {
	int32_t hash;
	Node* retrvd = nullptr;

	while (recur > 0) {
		hash = genHash(key);
		if (hash == -1) {
			errno = ENOENT;
			return -1;
		}
		if (lookup(hash, key, retrvd) == -1) {
			return -1;
		}
		if (retrvd->isNum) {
			data = retrvd->data.num_val;
			return 0;
		}
		key = retrvd->data.var_val;
		--recur;	
	}
	errno = ELOOP;
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
		hTable[i] = nullptr;
		fclose(logfile);
		logfile = fdopen(openat(log_dir, LOGFILE, O_RDWR|O_TRUNC), "w+");
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
	int fd = open(filename, O_WRONLY|O_CREAT|O_EXCL, S_IRWXU);
	if (fd == -1) {
		return -1; 
	}
	const size_t buff_size = 2*DEFAULT_SIZE + 3; //enough space to hold up to two var names and the formatting 
	char buffer[buff_size];
	int print_size;
	for (size_t i = 0; i < tblSize; ++i) {
		Node* current = hTable[i]; 
		while ( current != nullptr) {
			if(current->isNum){
				print_size = snprintf(buffer, buff_size, "%s=%ld\n", current->key, current->data.num_val);
			} else {
				print_size = snprintf(buffer, buff_size, "%s=%s\n", current->key, current->data.var_val);
			}
			if (print_size < 0) { 
				close(fd);
				return -1; 
			}
			if (write(fd, buffer, print_size) == -1) { 
				close(fd);
				return -1; 
			}
			current = current->next;
		}
	}
	close(fd);
	return 0;
}

void Hash::load_dump() {
	for (size_t i = 0; i < tblSize; ++i) {
		Node* current = hTable[i]; 
		while ( current != nullptr) {
			if(current->isNum) {
				fprintf(logfile, "%s=%ld\n", current->key, current->data.num_val);
			} else {
				fprintf(logfile, "%s=%s\n", current->key, current->data.var_val);
			}
			current = current->next;
		}
	}
	fflush(logfile);
}

/**
 * Load
 * @param filename: File to load entries from
 * @return: 0 if load is successful, -1 otherwise. Sets errno appropriately
 *
 * Parses the file and inserts '=' seperated key-value pairs into the hash table.
 * This function has the same potential failures as open(2) and read(2). If line
 * is preceeded by a `~` charecter, then call remove on the key read on that line.
 **/
int8_t Hash::load(const char *filename)
{
	FILE* fp = fopen(filename, "r");
	if (fp == nullptr) {
		return -1;
	}
	return load(fp);
}

int8_t Hash::load(FILE* fp) {
	uint8_t* key = nullptr;
	uint8_t* value = nullptr;
	char* endptr;
	int64_t int_val;
	int8_t chk_succ = 0;
	int vals_read;

	while ((vals_read = fscanf(fp," %m[^=]=%m[^\n]",&key,&value)) != EOF) {
		if(vals_read == 0) {
			errno = EINVAL;
			fclose(fp);
			return -1;
		}
		if (vals_read == 1) {
			if(key[0] == '~') {
				//remove may return an error, but is safe to ignore here.
				remove(key + 1);
				free(key);
				key = nullptr;
			} else {
				errno = EINVAL;
				free(key);
				fclose(fp);
				return -1;
			}
		} else {
			if (isalpha(value[0])) {
				chk_succ = insert(key, value);
			} else {
				int_val = strtol((char*)value, &endptr, 10);
				if (value[0] != '\0' && *endptr == '\0') {
					chk_succ = insert(key, int_val);
				} else {
					chk_succ = -1;
				} 
			}
			free(key);
			key = nullptr;
			free(value);
			value = nullptr;
			if (chk_succ == -1) {
				fclose(fp);
				return -1;
			}
		}
	}
	fclose(fp);
	return 0;
}

int8_t Hash::validate_key(const char* key) {
	if (strlen(key) < DEFAULT_SIZE) {
		std::regex exp("[a-z][a-z0-9_]*", std::regex_constants::icase);
		if (std::regex_match(key, exp))	{
			return 0;
		}
	}
	errno = EINVAL;
	return -1;
}

/**
 * This function sourced from
 * http://www.cse.yorku.ca/~oz/hash.html
 * @date 11/23/2020
 */
int32_t Hash::genHash(uint8_t *key)
{
	if (validate_key((char*) key) == -1) {

		return -1;
	}
	int32_t key_value = 5381;
	size_t size = strlen((char*)key);
	for(size_t i = 0; i < size; ++i) {
		key_value = ((key_value << 5) + key_value) + key[i]; /* hash * 33 + c */;
	}
	return floor(tblSize * ((key_value * HASHCONST) - floor(key_value * HASHCONST)));
}
