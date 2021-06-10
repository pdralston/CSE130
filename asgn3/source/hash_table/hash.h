/**
 * Hash Table Header File
 * Originally written 3/2017
 * @author Perry David Ralston Jr.
 * modified:
 * @date 11/12/2020
 */
#ifndef HASHH
#define HASHH

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <typeinfo>

class Node { // data class for the hashTable
	static const size_t MAX_KEY_LENGTH = 32;
  public:
	union data {
		int64_t num_val;
		uint8_t* var_val;
	} data;
	bool isNum;
	uint8_t* key;
	Node *next;
	template<class dataType>
	Node (uint8_t*, dataType, bool);
	~Node();
};

class Hash
{
  public:
	static const size_t DEFAULT_SIZE = 32;
	static const uint16_t DEFAULT_RECUR = 50;
	static constexpr double HASHCONST = .618034;
	static constexpr char* DEFAULT_LOG_DIR = (char*)"data";
	static constexpr char* LOGFILE = (char*)"logfile.log";
	Hash() : Hash(DEFAULT_SIZE){};
	Hash(size_t size, uint16_t recur = DEFAULT_RECUR, char* _logdir = DEFAULT_LOG_DIR);
	~Hash();
	template<class dataType>
	int8_t insert(uint8_t *, dataType);
	int8_t remove(uint8_t *);
	uint16_t get_recur_amt() { return recur_amt; }
	int8_t lookup(uint8_t *key, uint8_t*&);
	int8_t lookup(uint8_t *key, int64_t&);
	int8_t rlookup(uint8_t*, int64_t&, uint16_t recur);
	void clear();
	int8_t dump(const char *);
	int8_t load(const char *);
	size_t size() { return tblSize; }

  protected:
	bool init_load;
	int log_dir;
	FILE* logfile;
	Node **hTable;
	size_t tblSize;
	uint16_t recur_amt;
	int8_t validate_key(const char*);
	int32_t genHash(uint8_t *);
	template<class dataType>
	int8_t insert(int32_t, uint8_t *, dataType);
	int8_t remove(int32_t, uint8_t *);
	int8_t lookup(int32_t, uint8_t*, Node*&);
	int8_t load(FILE*);
	void load_dump();
	void sync_log() { fflush(logfile); }
};

template<class dataType>
Node::Node (uint8_t* nKey, dataType val, bool isNum) {
    key = (uint8_t*) calloc(MAX_KEY_LENGTH, sizeof(uint8_t));
	strcpy((char*)key, (char *)nKey);
	this->isNum = isNum;
	this->next = nullptr;
	if (isNum) {
	    data.num_val = (int64_t)val;
	} else {
	    data.var_val = (uint8_t*) calloc(MAX_KEY_LENGTH, sizeof(uint8_t));
		strcpy((char*)data.var_val, (char *)val);
	}
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
template<class dataType>
int8_t Hash::insert(uint8_t* key, dataType value)
{
	int32_t hash = genHash(key);
	if (hash == -1) {
		return -1;
	}
	if (insert<dataType>(hash, key, value) == 0) {
		if(!init_load) {
			if (typeid(dataType) == typeid(int64_t)) {
				fprintf(logfile, "%s=%ld\n", key, (int64_t)value);
			} else {
				fprintf(logfile, "%s=%s\n", key, (char*)value);
			}
			sync_log();
		}
		return 0;
	}
	return -1;
}

template<class dataType>
int8_t Hash::insert(int32_t hash, uint8_t* key, dataType value)
{
	Node* current = hTable[hash];
	Node* prev = hTable[hash];
	bool isNum = typeid(int64_t) == typeid(value);
	while(current != nullptr) {
		if (strcmp((char*)current->key, (char*)key) == 0) {
			if (!current->isNum) {
				free(current->data.var_val);
			}
			if (isNum) {
				current->data.num_val = (int64_t)value;
			} else {
				current->data.var_val = (uint8_t*) calloc(DEFAULT_SIZE, sizeof(uint8_t));
				strcpy((char*)current->data.var_val, (char *)value);
			}
			current->isNum = isNum;
			return 0;
		}
		prev = current;
		current = current->next;
	}
	Node* newNode;
	if (isNum) { 
		newNode = new Node(key, value, true);
	} else if (validate_key((char*)value) == 0) {
		newNode = new Node(key, value, false);
	} else {
		errno = EINVAL;
		return -1;
	} 
	if (prev == current) {
		hTable[hash] = newNode;
	} else {
		prev->next = newNode;
	}
	return 0;
}

#endif
