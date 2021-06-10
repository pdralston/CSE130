/**
 * hash table unit tests
 * 
 * @author Perry David Ralston Jr
 * @date 11/23/2020
 */

#include <assert.h>
#include <err.h>
#include <error.h>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "hash.h"

//Constants//
static const size_t TESTCOUNT = 18;
static const int8_t KEY_SIZE = 32;

//Test Functions//
void testNewHash(Hash*);
void testNewHashWithZeroArg(Hash*);
void testNewHashWithNonZeroArg(Hash*);
void testInsert(Hash*);
void testLookup(Hash*);
void testRemove(Hash*);
void testDump(Hash*);
void testDumpEmptyTable(Hash*);
void testLoad(Hash*);
void testClear(Hash*);
void testInsertWithVar(Hash*);
void testlookupWithVar(Hash*);
void testRecursiveLookUp(Hash*);
void testRecursiveLookUpTooManyLookUps(Hash*);
void testInitLoad(Hash*);
void testInitLoadWithVars(Hash*);
void testInitLoadWithRemoves(Hash*);
void testPersistance(Hash*);

//Data//
uint8_t key1[] = "foo",
		key2[] = "bar",
		key3[] = "foobar",
		*key,
		*var_result;
uint8_t badname[] = "this_is_a_really_really_bad_name_and_it_should_not_be_allowed_to_be_a_key",
		invalidName[] = "5oobar",
		invalidName_2[] = "foo~bar";
int64_t val1,
		val2,
		val3,
		result, 
		read_value;
std::ifstream in_gen;
std::string outfile, line;
enum class keys { key1, key2, key3 };
struct stat file_stats;

//Supporting Functions//
keys hash_switch(uint8_t*);


int main(){
  void (*fun_ptr[])(Hash*) = {
    testNewHash,
	testNewHashWithZeroArg,
	testNewHashWithNonZeroArg,
	testInsert,
	testLookup,
	testRemove,
	testDump,
	testDumpEmptyTable,
	testLoad,
	testClear,
	testInsertWithVar,
	testlookupWithVar,
	testRecursiveLookUp,
	testRecursiveLookUpTooManyLookUps,
	testInitLoad,
	testInitLoadWithVars,
	testInitLoadWithRemoves,
	testPersistance
  };
  
  for(size_t i = 0; i < TESTCOUNT; i++) {
    //before:
    Hash* H = new Hash();
	val1 = 42;
	val2 = -42;
	val3 = 128;
	result = read_value = 0;
	key = (uint8_t*) calloc(KEY_SIZE, sizeof(uint8_t));
	var_result = (uint8_t*) calloc(KEY_SIZE, sizeof(uint8_t));
    
    //test:
	if (i == 10) {
		printf("-----Beginning Asgn3 Specific Tests:-----\n");
	}
    fun_ptr[i](H);
    
    //clean:
    delete(H);
	free(key);
	free(var_result);
  }
  printf("All tests passed successfully\n");
  return 0;
}

keys hash_switch(uint8_t* in_str) {
	if (strcmp((char*)in_str, (char*)key1) == 0) {
		return keys::key1;
	}
	if (strcmp((char*)in_str, (char*)key2) == 0) {
		return keys::key2;
	}
	if (strcmp((char*)in_str, (char*)key3) == 0) {
		return keys::key3;
	}
	err(EXIT_FAILURE, "hash_switch(): %s",strerror(EINVAL));
}

void testNewHash(Hash* H) {
	printf("TestNewHash: ");
	assert(H->size() == Hash::DEFAULT_SIZE);
	printf("Passed\n");
}

void testNewHashWithZeroArg(Hash* H) {
	printf("TestNewHashWithZeroArg: ");
	size_t size = 0;
	H = new Hash(size);
	assert(H->size() == Hash::DEFAULT_SIZE);
	delete(H);
	printf("Passed\n");
}

void testNewHashWithNonZeroArg(Hash* H) {
	printf("TestNewHashWithNonZeroArg: ");
	size_t size = 100;
	H = new Hash(size);
	assert(H->size() == size);
	delete(H);
	printf("Passed\n");
}

void testInsert(Hash* H) {
	printf("TestInsert: ");
	assert(H->insert(key1, val1) != -1);
	assert(H->insert(badname, val1) == -1);
	assert(H->insert(invalidName, val1) == -1);
	assert(H->insert(invalidName_2, val1) == -1);
	H = new Hash(1);
	assert(H->insert(key1, val1) != -1);
	assert(H->insert(key2, val2) != -1);
	delete(H);
	printf("Passed\n");
}

void testLookup(Hash* H) {
	printf("TestLookup: ");
	H->insert(key1, val1);
	assert(H->lookup(key1, result) != -1);
	assert(result == val1);
	assert(H->lookup(key2, result) == -1);
	assert(H->lookup(badname, result) == -1);
	assert(H->lookup(invalidName, result) == -1);
	assert(H->lookup(invalidName_2, result) == -1);
	H = new Hash(1);
	H->insert(key1, val1);
	assert(H->lookup(key1, result) == 0);
	assert(result == val1);
	H->insert(key2, val2);
	assert(H->lookup(key2, result) == 0);
	assert(result == val2);
	delete(H);
	printf("Passed\n");
}

void testRemove(Hash* H) {
	printf("TestRemove: ");
	H->insert(key1, val1);
	assert(H->remove(key1) == 0);
	assert(H->remove(key2) == -1);
	assert(H->lookup(key1, result) == -1);
	printf("Passed\n");
}

void testDump(Hash* H) {
	printf("TestDump: ");
	H->insert(key1, val1);
	H->insert(key2, val2);
	H->insert(key3, val3);
	outfile = "outfile.txt";
	assert(H->dump(outfile.c_str()) != -1);
	assert(H->dump(outfile.c_str()) == -1);

	in_gen.open(outfile);
	assert(in_gen.good());
	while (getline(in_gen, line)) {
		assert(sscanf(line.c_str(), "%[^=]=%ld", key, &read_value) == 2);
		switch (hash_switch(key)) {
			case keys::key1: {
				assert(H->lookup(key1, result) != -1);
				break;
			}
			case keys::key2: {
				assert(H->lookup(key2, result) != -1);
				break;
			}
			case keys::key3: {
				assert(H->lookup(key3, result) != -1);
				break;
			}
			default: {
				assert(false);
			}
		}
		assert(result == read_value);
	}
	in_gen.close();
	//delete the outfile
	if (remove(outfile.c_str()) == -1) {
		warn("Unable to delete %s: %s", outfile.c_str(), strerror(errno));
	}
	printf("Passed\n");
}

void testDumpEmptyTable(Hash* H) {
	printf("TestDumpEmptyTable: ");
	outfile = "outfile.txt";

	assert(H->dump(outfile.c_str()) != -1);
	assert(stat(outfile.c_str(), &file_stats) == 0);
	assert(file_stats.st_size == 0);
	
	if (remove(outfile.c_str()) == -1) {
		warn("Unable to delete %s: %s", outfile.c_str(), strerror(errno));
	}
	printf("Passed\n");
}

void testLoad(Hash* H) {
	printf("TestLoad: ");
	//all entries are good
	assert(H->load("data/sample_in.txt") == 0);
	assert(H->lookup(key1, result) == 0);
	assert(result == val1);
	assert(H->lookup(key2, result) == 0);
	assert(result == val2);
	assert(H->lookup(key3, result) == 0);
	assert(result == val3);
	H->remove(key1);
	H->remove(key2);
	H->remove(key3);

	//file does not exist
	assert(H->load("does_not_exist.txt") == -1);

	//file contains a bad key name
	assert(H->load("data/bad_key_name_with_good.txt") == -1);
	H->lookup(key1, result);
	assert(result == val1);
	H->lookup(key2, result);
	assert(result == val2);
	printf("Passed\n");
}

void testClear(Hash* H) {
	printf("TestClear: ");
	H->insert(key1, val1);
	H->clear();
	outfile = "outfile.txt"; 
	H->dump(outfile.c_str());
	assert(stat(outfile.c_str(), &file_stats) == 0);
	assert(file_stats.st_size == 0);
	if (remove(outfile.c_str()) == -1) {
		warn("Unable to delete %s: %s", outfile.c_str(), strerror(errno));
	}
	printf("Passed\n");
}

/*----------ASGN3 TESTS --------------*/

void testInsertWithVar(Hash* H) {
	printf("testInsertWithVar: ");
	assert(H->insert(key1, key2) == 0);
	printf("Passed\n");
}

void testlookupWithVar(Hash* H) {
	printf("testlookupWithVar: ");
	H->insert(key1, key2);
	assert(H->lookup(key1, var_result) == 0);
	assert(strcmp((char*)key2, (char*)var_result) == 0);
	printf("Passed\n");
}

void testRecursiveLookUp(Hash* H) {
	printf("testRecursiveLookUp: ");
	H->insert(key1, key2);
	H->insert(key2, val1);
	assert(H->rlookup(key1, result, H->get_recur_amt()) == 0);
	assert(result == val1);
	printf("Passed\n");
}

void testRecursiveLookUpTooManyLookUps(Hash* H) {
	printf("testRecursiveLookUpTooManyLookUps: ");
	H->insert(key1, key2);
	assert(H->rlookup(key1, result, H->get_recur_amt()) == -1);
	printf("Passed\n");
}

void testInitLoad(Hash* H) {
	printf("TestInitLoad: ");
	std::system("cp data/sample_in.txt data/logfile.log");
	H = new Hash();
	assert(H->lookup(key1, result) == 0);
	assert(result == val1);
	assert(H->lookup(key2, result) == 0);
	assert(result == val2);
	assert(H->lookup(key3, result) == 0);
	assert(result == val3);
	delete(H);
	printf("Passed\n");
}

void testInitLoadWithVars(Hash* H) {
	printf("testInitLoadWithVars: ");
	std::system("cp data/input_with_vars.txt data/logfile.log");
	H = new Hash();
	assert(H->lookup(key1, result) == 0);
	assert(result == val1);
	assert(H->lookup(key2, var_result) == 0);
	assert(strcmp((char*)var_result, (char*)key3) == 0);
	assert(H->lookup(key3, result) == 0);
	assert(result == val3);
	delete(H);
	printf("Passed\n");
}

void testInitLoadWithRemoves(Hash* H) {
	printf("testInitLoadWithRemoves: ");
	std::system("cp data/input_with_removes.txt data/logfile.log");
	H = new Hash();
	assert(H->lookup(key1, result) == -1);
	assert(H->lookup(key3, result) == 0);
	assert(result == val3);
	delete(H);
	printf("Passed\n");
}

void testPersistance(Hash* H) {
	printf("testPersistance: ");
	H->insert(key1, val1);
	H->insert(key2, val2);
	H->insert(key3, val3);
	H->insert(key3, key1);
	H = new Hash();
	assert(H->lookup(key1, result) == 0);
	assert(result = val1);
	assert(H->lookup(key2, result) == 0);
	assert(result = val2);
	assert(H->lookup(key3, var_result) == 0);
	assert(strcmp((char*)key1, (char*)var_result) == 0);
	assert(H->rlookup(key3, result, H->get_recur_amt()) == 0);
	assert(result = val1);
	delete(H);
	printf("Passed\n");
}


