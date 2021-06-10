/**
 * Hash Table Header File
 * Originally written 3/2017
 * @author Perry David Ralston Jr.
 * modified:
 * @date 11/12/2020
 */
#ifndef HASHH
#define HASHH

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

class Node { // data class for the hashTable
	static const size_t MAX_KEY_LENGTH = 32;

  public:
	int64_t data;
	uint8_t* key;
	Node *next;
	Node (uint8_t*, int64_t);
	~Node();
};

class Hash
{
  public:
	static const size_t DEFAULT_SIZE = 32;
	static constexpr double HASHCONST = .618034;
	Hash() : Hash(DEFAULT_SIZE){};
	Hash(size_t size);
	void replace(uint8_t *key, int64_t &value) { return insert(key, value); }
	void insert(uint8_t *, int64_t);
	int8_t remove(uint8_t *);
	int8_t lookup(uint8_t *, int64_t &);
	void clear();
	int8_t dump(const char *);
	int8_t load(const char *);

  protected:
	size_t tblSize;
	Node **hTable;
	uint32_t genHash(uint8_t *);
};

#endif
