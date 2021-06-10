/** Bounded_Buffer source file
 *  Bounded class to maintain a buffer for reading and writing
 *  client messages
 *
 *  @author Perry David Ralston Jr.
 *  @date 10/28/2020
 */

#include "bounded_buffer.h"
#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

Bounded_Buffer::Bounded_Buffer()
{
	root = (uint8_t *)calloc(BUFFER_SIZE, sizeof(uint8_t));
	start = end = root;
	client_fd = 0;
}

Bounded_Buffer::~Bounded_Buffer()
{
	free(root);
	root = start = end = NULL;
}

void Bounded_Buffer::close_cl()
{
	close(client_fd);
	client_fd = 0;
}

ssize_t Bounded_Buffer::fill()
{
	if(start != root)
		start = root;
	if(client_fd == 0)
		return -1;
	ssize_t bytes_read = recv(client_fd, root, BUFFER_SIZE, 0);
	if(bytes_read == -1) {
		err(EXIT_FAILURE, "%s", strerror(errno));
	}
	if(bytes_read == 0) {
		close_cl();
		return -1;
	}
	// end points at 1 past the end of the buffer
	end = root + bytes_read;
	return bytes_read;
}

ssize_t Bounded_Buffer::from_file(int file_d, ssize_t size)
{
	ssize_t bytes_read = 0, curr_read = 0;
	size_t bytes_rem = size;
	flush();
	while(bytes_rem > 0) {
		size_t read_size = (BUFFER_SIZE > bytes_rem) ? bytes_rem : BUFFER_SIZE;
		curr_read = read(file_d, start, read_size);
		if(curr_read == -1) {
			bytes_read = -1;
			break;
		}
		end = root + curr_read;
		bytes_read += curr_read;
		bytes_rem = size - bytes_read;
		if(flush() == -1) {
			bytes_read = -1;
			break;
		}
	}
	return bytes_read;
}

void Bounded_Buffer::dump(uint16_t dump_size)
{
	uint16_t bytes_dumped = 0;
	while(bytes_dumped < dump_size) {
		bytes_dumped += fill();
		clear();
	}
}

ssize_t Bounded_Buffer::flush()
{
	if(start == end) {
		// buffer is empty start and end may not be set to root
		clear();
		return 0;
	}
	ssize_t flush_length = end - start;
	if(client_fd == 0)
		return -1;
	ssize_t bytes_sent = send(client_fd, start, flush_length, 0);
	if(bytes_sent == -1) {
		err(EXIT_FAILURE, "%s", strerror(errno));
	}
	if(bytes_sent < flush_length) {
		close_cl();
		return -1;
	}
	clear();
	return bytes_sent;
}

ssize_t Bounded_Buffer::to_file(int file_d, ssize_t size)
{
	ssize_t bytes_written = write(file_d, start, end - start);
	if(bytes_written == -1) {
		return bytes_written;
	}
	while(bytes_written != size) {
		fill();
		if(write(file_d, start, end - start) == -1)
			return -1;
		bytes_written += (end - start);
	}
	return bytes_written;
}

uint8_t *Bounded_Buffer::getByte()
{
	if(isEmpty()) {
		if(fill() == -1)
			return NULL;
	}
	uint8_t *ret_val = start;
	++start;
	return ret_val;
}

int8_t Bounded_Buffer::getBytes(size_t size, uint8_t *dest)
{
	uint8_t *curr_byte = NULL;
	for(size_t i = 0; i < size; ++i) {
		curr_byte = getByte();
		if(curr_byte == NULL) {
			return -1;
		}
		dest[i] = *curr_byte;
	}
	return 0;
}

int8_t Bounded_Buffer::pushByte(uint8_t in_byte)
{
	if(end == root + BUFFER_SIZE) {
		if(flush() == -1) {
			return -1;
		}
	}
	end[0] = in_byte;
	++end;
	return 0;
}

int8_t Bounded_Buffer::pushBytes(size_t size, uint8_t *in_bytes)
{
	for(size_t i = 0; i < size; ++i) {
		if(pushByte(in_bytes[i]) == -1)
			return -1;
	}
	return 0;
}