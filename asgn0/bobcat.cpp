/**
 * bobcat.cpp
 *
 * Emulates the behavior of cat(2) sans flags
 * reads from given filepaths and prints the contents
 * to stdout.
 *
 * @author Perry David Ralston Jr.
 * @date 10/8/2020
 **/

#include <cstdint>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const int STDOUT = 1;
const int STDIN = 0;
const size_t BUFFER_SIZE = 8000;
const int UNDEFINED = -2;

int8_t read_file(int fd);

int main(int argc, char const *argv[])
{
	if(argc == 1) {
		if(read_file(STDIN) == -1) {
			fprintf(stderr, "bobcat: %s\n", strerror(errno));
			return (errno);
		}
	}

	bool read_stdin = false;
	int fd = UNDEFINED;
	for(int i = 1; i < argc; i++) {
		if(!read_stdin && strcmp(argv[i], "-") == 0) {
			fd = STDIN;
			read_stdin = true;
		}

		if(strcmp(argv[i], "-") != 0) {
			fd = open(argv[i], O_RDONLY);
			if(fd == -1) {
				fprintf(stderr, "bobcat: %s: %s\n", argv[i], strerror(errno));
				return (errno);
			}
		}

		if(fd != UNDEFINED) {
			if(fd == -1 || read_file(fd) == -1 || close(fd) == -1) {
				fprintf(stderr, "bobcat: %s\n", strerror(errno));
				return (errno);
			}
		}
		fd = UNDEFINED;
	}
	return (0);
}

int8_t read_file(int fd)
{
	char *buffer = (char *)calloc(BUFFER_SIZE, sizeof(char));
	ssize_t bytes_read = read(fd, buffer, BUFFER_SIZE);
	while(bytes_read > 0) {
		if(write(STDOUT, buffer, bytes_read) == -1) {
			return -1;
		}
		bytes_read = read(fd, buffer, BUFFER_SIZE);
	}
	if(bytes_read == -1) {
		return -1;
	}
	return 0;
}
