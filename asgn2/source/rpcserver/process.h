#ifndef THREAD_PROCESS
#define THREAD_PROCESS
 /**
 * process
 *
 * Main processing instructions for a thread
 *
 * @author Perry David Ralston Jr.
 * @date 11/18/2020
 **/

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

#include "bounded_buffer.h"
#include "hash.h"
#include "math_funcs.h"
#include "structs.h"


template<class var_int_type>
void conv_to_nbo(Bounded_Buffer &, var_int_type);
template<class var_int_type>
var_int_type conv_frm_nbo(Bounded_Buffer &);
void build_header(resp_header &, Bounded_Buffer &);
void send_header(resp_header &, Bounded_Buffer &);
void send_math_response(resp_header &, Bounded_Buffer &, int64_t);
int64_t read_file(char *, uint64_t, uint16_t, Bounded_Buffer &, resp_header &);
int64_t write_file(char *, uint64_t, uint16_t, Bounded_Buffer &, resp_header &);
int64_t create(char *);
int64_t filesize(char *);
int8_t getVarName(Bounded_Buffer&, uint8_t*, int8_t);
int8_t getVar(Bounded_Buffer&, SyncHash*, int64_t&, int64_t);

static const int16_t MATH_OPS = 0x0100;
static const int16_t FINAL_BYTE = 0x000f;
static const int16_t VAR_A = 0x10;
static const int16_t VAR_B = 0x20;
static const int16_t VAR_RES = 0x40;

void process(int cl, int64_t ident, SyncHash* htable){
	Bounded_Buffer buffer;
	resp_header header;
	int64_t arith_arg1, arith_arg2, arith_result, offset, file_size;
	uint16_t filename_size = 0, buff_size = 0;
	uint8_t* filename;
	buffer.client_fd = cl;
	while(buffer.client_fd != 0) {
		build_header(header, buffer);
		if(buffer.client_fd == 0)
			break;
		if ((header.op & MATH_OPS) != 0) {
			int16_t math_op = (header.op & MATH_OPS) + (header.op & FINAL_BYTE);
			if (header.op & VAR_RES) {
				//acquire lock
			}
			//this block is bad and I feel bad about it
			if(header.op != 0x010f) {
				if((header.op & VAR_A) != 0) {
					if (getVar(buffer, htable, arith_arg1, ident) == -1) {
						header.err_code = errno;
						send_header(header, buffer);
						break;
					}
				} else {
					arith_arg1 = conv_frm_nbo<uint64_t>(buffer);
				}
				if ((header.op & VAR_B) != 0) {
					if (getVar(buffer, htable, arith_arg2, ident) == -1) {
						header.err_code = errno;
						send_header(header, buffer);
						break;
					}
				} else {
					arith_arg2 = conv_frm_nbo<uint64_t>(buffer);
				}
				if ((header.op & VAR_RES) != 0) {
					filename = (uint8_t*)calloc(SyncHash::DEFAULT_SIZE, sizeof(uint8_t));
					uint8_t size = conv_frm_nbo<uint8_t>(buffer);
					if (getVarName(buffer, filename, size) == -1) {
						header.err_code = errno;
						send_header(header, buffer);
						free(filename);
						break;
					}
					htable->acquire(filename, ident);
				}
			}
			switch(math_op) {
				case 0x0101: // add
					if(buffer.client_fd != 0) {
						if(add(arith_arg1, arith_arg2, arith_result) == -1) {
							header.err_code = EINVAL;
							send_header(header, buffer);
						} else {
							if ((header.op & VAR_RES) != 0) {
								htable->insert(filename, arith_result, ident);
								htable->release(filename);
								free(filename);
							}
							send_math_response(header, buffer, arith_result);
						}
					}
					break;
				case 0x0102: // sub
					if(buffer.client_fd != 0) {
						if(subtract(arith_arg1, arith_arg2, arith_result) == -1) {
							header.err_code = EINVAL;
							send_header(header, buffer);
						} else {
							if ((header.op & VAR_RES) != 0) {
								htable->insert(filename, arith_result, ident);
								htable->release(filename);
								free(filename);
							}
							send_math_response(header, buffer, arith_result);
						}
					}
					break;
				case 0x0103: // mult
					if(buffer.client_fd != 0) {
						if(multiply(arith_arg1, arith_arg2, arith_result) == -1) {
							header.err_code = EINVAL;
							send_header(header, buffer);
						} else {
							if ((header.op & VAR_RES) != 0) {
								htable->insert(filename, arith_result, ident);
								htable->release(filename);
								free(filename);
							}
							send_math_response(header, buffer, arith_result);
						}
					}
					break;
				case 0x0104: // div
					if(buffer.client_fd != 0) {
						if(div(arith_arg1, arith_arg2, arith_result) == -1) {
							header.err_code = EINVAL;
							send_header(header, buffer);
						} else {
							if ((header.op & VAR_RES) != 0) {
								htable->insert(filename, arith_result, ident);
								htable->release(filename);
								free(filename);
							}
							send_math_response(header, buffer, arith_result);
						}
					}
					break;
				case 0x0105: // mod
					if(buffer.client_fd != 0) {
						if(mod(arith_arg1, arith_arg2, arith_result) == -1) {
							header.err_code = EINVAL;
							send_header(header, buffer);
						} else {
							if ((header.op & VAR_RES) != 0) {
								htable->insert(filename, arith_result, ident);
								htable->release(filename);
								free(filename);
							}
							send_math_response(header, buffer, arith_result);
						}
					}
					break;
				case 0x010f: {// del
					filename = (uint8_t*)calloc(SyncHash::DEFAULT_SIZE, sizeof(uint8_t));
					uint8_t size = conv_frm_nbo<uint8_t>(buffer);
					if (getVarName(buffer, filename, size) == -1) {
						header.err_code = errno;
						send_header(header, buffer);
						free(filename);
						break;
					}
					if (htable->remove(filename, ident) == -1) {
						header.err_code = ENOENT;
						send_header(header, buffer);
						free(filename);
						break;
					}
					free(filename);
					send_header(header, buffer);
					break;
				}
				default:
					header.err_code = EBADRQC;
					send_header(header, buffer);
			}
		} else {
			switch(header.op) {
				case 0x0201: // read
				case 0x0202: // write
					filename_size = conv_frm_nbo<uint16_t>(buffer);
					filename = (uint8_t *)calloc(filename_size + 1, sizeof(uint8_t));
					buffer.getBytes(filename_size, filename);
					filename[filename_size] = '\0';
					offset = conv_frm_nbo<uint64_t>(buffer);
					buff_size = conv_frm_nbo<uint16_t>(buffer);
					if((header.op == 0x0201
							? read_file((char *)filename, offset, buff_size, buffer, header)
							: write_file((char *)filename, offset, buff_size, buffer, header))
						== -1) {
						header.err_code = errno;
						send_header(header, buffer);
					}
					free(filename);
					break;
				case 0x0210: // create
					filename_size = conv_frm_nbo<uint16_t>(buffer);
					filename = (uint8_t *)calloc(filename_size + 1, sizeof(uint8_t));
					buffer.getBytes(filename_size, filename);
					filename[filename_size] = '\0';
					if(create((char *)filename) == -1) {
						header.err_code = errno;
						send_header(header, buffer);
					} else {
						send_header(header, buffer);
					}
					free(filename);
					break;
				case 0x0220: // filesize
					filename_size = conv_frm_nbo<uint16_t>(buffer);
					filename = (uint8_t *)calloc(filename_size + 1, sizeof(uint8_t));
					buffer.getBytes(filename_size, filename);
					filename[filename_size] = '\0';
					file_size = filesize((char *)filename);
					if(file_size == -1) {
						header.err_code = errno;
						send_header(header, buffer);
					} else {
						send_header(header, buffer);
						conv_to_nbo<int64_t>(buffer, file_size);
						buffer.flush();
					}
					free(filename);
					break;
				case 0x0301: //dump
					filename_size = conv_frm_nbo<uint16_t>(buffer);
					filename = (uint8_t *)calloc(filename_size + 1, sizeof(uint8_t));
					buffer.getBytes(filename_size, filename);
					filename[filename_size] = '\0';
					if (htable->dump((char*)filename, ident) == -1) {
						header.err_code = errno;
						send_header(header, buffer);
					} else {
						send_header(header, buffer);
					}
					free(filename);
					break;
				case 0x0302: //load
					filename_size = conv_frm_nbo<uint16_t>(buffer);
					filename = (uint8_t *)calloc(filename_size + 1, sizeof(uint8_t));
					buffer.getBytes(filename_size, filename);
					filename[filename_size] = '\0';
					if (htable->load((char*)filename, ident) == -1) {
						header.err_code = errno;
						send_header(header, buffer);
					} else {
						send_header(header, buffer);
					}
					free(filename);
					break;
				default:
					header.err_code = EBADRQC;
					send_header(header, buffer);
			}
		}
	}
}

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
var_int_type conv_frm_nbo(Bounded_Buffer &b_buff)
{
	var_int_type ret_val = 0;
	for(size_t i = 0; i < sizeof(ret_val); ++i) {
		uint8_t *curr_byte = b_buff.getByte();
		if(curr_byte == NULL) {
			return -1;
		}
		ret_val = (ret_val << 8) + *curr_byte;
	}
	return ret_val;
}

void build_header(resp_header &header, Bounded_Buffer &b_buff)
{
	header.op = conv_frm_nbo<uint16_t>(b_buff);
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

int8_t getVarName(Bounded_Buffer& buffer, uint8_t* name, int8_t size) {
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

int8_t getVar(Bounded_Buffer &buffer, SyncHash* htable, int64_t &value, int64_t ident) {
	int8_t size = conv_frm_nbo<int8_t>(buffer);
	uint8_t* key = (uint8_t*)calloc(SyncHash::DEFAULT_SIZE, sizeof(uint8_t));
	if (getVarName(buffer, key, size) == -1) {
		free(key);
		return -1;
	}
	if(htable->lookup(key, value, ident) == -1) {
		errno = ENONET;
		free(key);
		return -1;
	}
	free(key);
	return 0;
}

#endif
