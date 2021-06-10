#ifndef PROCESS_FUNCS
#define PROCESS_FUNCS

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct thread Thread;

template<class var_int_type>
void conv_to_nbo(Bounded_Buffer &, var_int_type);
template<class var_int_type>
int8_t conv_frm_nbo(Bounded_Buffer &, var_int_type&);
void build_header(resp_header &, Bounded_Buffer &);
void send_header(resp_header &, Bounded_Buffer &);
void send_err_header(resp_header &, Bounded_Buffer &);
void send_math_response(resp_header &, Bounded_Buffer &, int64_t);
int64_t read_file(char *, uint64_t, uint16_t, Bounded_Buffer &, resp_header &);
int64_t write_file(char *, uint64_t, uint16_t, Bounded_Buffer &, resp_header &);
int64_t create(char *);
int64_t filesize(char *);
int8_t getVarName(Bounded_Buffer&, uint8_t*);
int8_t getVar(Bounded_Buffer&, SyncHash*, int64_t&, int64_t);
void free_var_args(uint8_t**&);
void handle_math_arg_error(resp_header&, Bounded_Buffer&, Thread*, uint8_t**&, ssize_t&);

static const int16_t MATH_OPS = 0x0100;
static const int16_t CLEAR_OP = 0x0310;
static const int16_t FINAL_BYTE = 0x000f;
static const uint32_t CLEAR_CONFIRM = 0x0badbad0;
static const int16_t VAR_A = 0x10;
static const int16_t VAR_B = 0x20;
static const int16_t VAR_RES = 0x40;
static const int16_t RECURSE = 0x80;
static const size_t ARG_COUNT = 3;
static const int16_t FLAGS[ARG_COUNT] = {VAR_A, VAR_B, VAR_RES};

template<class var_int_type>
void conv_to_nbo(Bounded_Buffer &b_buff, var_int_type to_conv)
{
	uint8_t to_push = 0;
	errno = 0;
	ssize_t size = sizeof(to_conv) - 1;
	for(ssize_t i = size; 0 <= i; --i) {
		to_push = (to_conv >> 8 * i) & 0xFF;
		if(b_buff.pushByte(to_push) == -1) {
			// TODO error handling
		}
	}
}

template<class var_int_type>
int8_t conv_frm_nbo(Bounded_Buffer &b_buff, var_int_type& value)
{
	value = 0;
	for(size_t i = 0; i < sizeof(value); ++i) {
		uint8_t *curr_byte = b_buff.getByte();
		if(curr_byte == NULL) {
			errno = EINVAL;
			return -1;
		}
		value = (value << 8) + *curr_byte;
	}
	return 0;
}

void build_header(resp_header &header, Bounded_Buffer &b_buff)
{
	conv_frm_nbo<uint16_t>(b_buff, header.op);
	if(b_buff.client_fd == 0)
		return;
	b_buff.getBytes(sizeof(uint32_t), header.req_ident);
	if(b_buff.client_fd == 0)
		return;
	header.err_code = 0;
}

void send_header(resp_header &header, Bounded_Buffer &b_buff)
{
	b_buff.clear();
	b_buff.pushBytes(sizeof(uint32_t), header.req_ident);
	if(b_buff.client_fd == 0)
		return;
	b_buff.pushByte(header.err_code);
	if(b_buff.client_fd == 0)
		return;
	b_buff.flush();
}

void send_err_header(resp_header &header, Bounded_Buffer &b_buff) {
	header.err_code = errno;
	send_header(header, b_buff);
}

void send_math_response(resp_header &header, Bounded_Buffer &b_buff, int64_t data)
{
	if(b_buff.client_fd == 0)
		return;
	send_header(header, b_buff);
	if(b_buff.client_fd == 0)
		return;
	conv_to_nbo(b_buff, data);
	b_buff.flush();
}

int64_t read_file(char *filename,
  uint64_t offset,
  uint16_t bufsize,
  Bounded_Buffer &b_buff,
  resp_header &header)
{
	int file_d = open(filename, O_RDONLY | O_EXCL);
	if(file_d == -1) {
		b_buff.dump(bufsize);
		return -1;
	}
	int64_t file_size = filesize(filename);
	if(file_size == -1) {
		close(file_d);
		return -1;
	}
	if((int64_t)offset > file_size || (file_size - offset) < bufsize) {
		close(file_d);
		errno = EINVAL;
		return -1;
	}
	if(lseek(file_d, offset, SEEK_SET) == -1) {
		close(file_d);
		return -1;
	}
	send_header(header, b_buff);
	conv_to_nbo<uint16_t>(b_buff, bufsize);
	int64_t bytes_read = b_buff.from_file(file_d, bufsize);
	close(file_d);
	if(bytes_read == -1)
		return -1;
	return bytes_read;
}

int64_t write_file(char *filename,
  uint64_t offset,
  uint16_t bufsize,
  Bounded_Buffer &b_buff,
  resp_header &header)
{
	int file_d = open(filename, O_WRONLY | O_EXCL);
	if(file_d == -1) {
		b_buff.dump(bufsize);
		return -1;
	}
	if(lseek(file_d, offset, SEEK_SET) == -1) {
		close(file_d);
		return -1;
	}
	int64_t bytes_written = b_buff.to_file(file_d, bufsize);
	close(file_d);
	if(bytes_written == -1)
		return -1;
	send_header(header, b_buff);
	return bytes_written;
}

int64_t create(char *filename)
{
	int fd = open(filename, O_RDWR | O_CREAT | O_EXCL, S_IRWXU);
	if(fd == -1 || close(fd) == -1)
		return -1;
	return 0;
}

int64_t filesize(char *filename)
{
	struct stat file_stats;
	if(stat(filename, &file_stats) == -1) {
		return -1;
	}
	return file_stats.st_size;
}

int8_t getVarName(Bounded_Buffer& buffer, uint8_t* name) {
	int8_t size = 0;
	conv_frm_nbo<int8_t>(buffer, size);
	if (size <= 0 || (size_t)size >= SyncHash::DEFAULT_SIZE) {
		errno = EINVAL;
		return -1;
	}
	if (buffer.getBytes(size, name) == -1) {
		errno = EINVAL;
		return -1;
	}
	name[size] = '\0';
	return 0;
}

void free_var_args(uint8_t**& var_args){
	for (size_t i = 0; i < ARG_COUNT; ++i) {
		if (var_args[i] != nullptr) {
			free(var_args[i]);
			var_args[i] = nullptr;
		}
	}
}

void handle_math_arg_error(resp_header& header, Bounded_Buffer& buffer, Thread* self,uint8_t**& var_args, ssize_t& fun_index) {
	send_err_header(header, buffer);
	self->hTable->release(var_args, ARG_COUNT, self->ident);
	free_var_args(var_args);
	fun_index = -1;
}

#endif
