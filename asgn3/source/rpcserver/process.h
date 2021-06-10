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
#include "process_funcs.h"
#include "structs.h"

void process(Thread* self){
	Bounded_Buffer buffer;
	resp_header header;
	uint64_t offset; 
	int64_t file_size;
	int64_t arith_args[ARG_COUNT] = {0,0,0};
	int16_t math_op;
	uint16_t filename_size = 0, buff_size = 0;
	uint8_t *filename, *keyname, *var_result, var_name_size;
	uint8_t** var_args = (uint8_t**)calloc(ARG_COUNT, sizeof(char*));
	buffer.client_fd = self->cl;
	ssize_t fun_index;

	while(buffer.client_fd != 0) {
		build_header(header, buffer);
		if(header.op == CLEAR_OP) {
			uint32_t confirm;
			if(conv_frm_nbo(buffer, confirm) == -1 || confirm != CLEAR_CONFIRM) {
				errno = EINVAL;
				send_err_header(header, buffer);
			}
			self->hTable->clear(self->ident);
			send_header(header, buffer);
		} else if ((header.op & MATH_OPS) != 0) {
			math_op = (header.op & MATH_OPS) + (header.op & FINAL_BYTE);
			fun_index = (math_op & FINAL_BYTE) - 1;
			if (fun_index < MATH_FUNC_COUNT) {
				for (size_t i = 0; i < ARG_COUNT && fun_index != -1; ++i) {
					if ((header.op & FLAGS[i]) != 0) {
						var_args[i] = (uint8_t*)calloc(SyncHash::DEFAULT_SIZE, sizeof(uint8_t));
						if (getVarName(buffer, var_args[i]) == -1) {
							handle_math_arg_error(header, buffer, self, var_args, fun_index);
						}
					} else if (i != ARG_COUNT - 1) {
						if (conv_frm_nbo<int64_t>(buffer, arith_args[i]) == -1) {
							handle_math_arg_error(header, buffer, self, var_args, fun_index);
						}
					}
				}
				if (var_args[2] != nullptr && fun_index != -1) {
					//if the result is one of the args then operation must be atomic
					if(self->hTable->acquire(var_args, ARG_COUNT, self->ident) == -1) {
						handle_math_arg_error(header, buffer, self, var_args, fun_index);
					}
				}
				//set the arithmetic variable arguments
				for (size_t i = 0; i < ARG_COUNT - 1  && fun_index != -1; ++i) {
					if (var_args[i] != nullptr) {
						if ((header.op & RECURSE) == 0) {
							//do a simple lookup
							if (self->hTable->lookup(var_args[i], arith_args[i], self->ident) == -1) {
								handle_math_arg_error(header, buffer, self, var_args, fun_index);
							}
						} else {
							//do a recursive lookup
							if (self->hTable->rlookup(var_args[i], arith_args[i], self->ident) == -1) {
								handle_math_arg_error(header, buffer, self, var_args, fun_index);
							}
						}
					}
				}
				if (fun_index != -1) {
					if (math_fun[fun_index](arith_args[0], arith_args[1], arith_args[2]) == -1) {
						handle_math_arg_error(header, buffer, self, var_args, fun_index);
					} else {
						if (var_args[2] != nullptr) {
							if (self->hTable->insert(var_args[2], arith_args[2], self->ident) == -1){
								send_err_header(header, buffer);
							}
						}
						send_math_response(header, buffer, arith_args[2]);
						self->hTable->release(var_args, ARG_COUNT, self->ident);
						free_var_args(var_args);
					}
				}
			} else {
				keyname = (uint8_t*)calloc(SyncHash::DEFAULT_SIZE, sizeof(uint8_t));
				if (getVarName(buffer, keyname) == -1) {
					send_err_header(header, buffer);
					free(keyname);
					break;
				}
				switch (math_op) {
					case 0x010f: {// del
						if (self->hTable->remove(keyname, self->ident) == -1) {
							send_err_header(header, buffer);
							free(keyname);
							break;
						}
						free(keyname);
						send_header(header, buffer);
						break;
					}
					case 0x0108: { //getv
						var_result = (uint8_t*)calloc(SyncHash::DEFAULT_SIZE, sizeof(uint8_t));
						if (self->hTable->lookup(keyname, var_result, self->ident) == -1) {
							send_err_header(header, buffer);
						} else {
							send_header(header, buffer);
							var_name_size = strlen((char*)var_result);
							buffer.pushByte(var_name_size);
							buffer.pushBytes(var_name_size, var_result);
							buffer.flush();
						}
						free(var_result);
						free(keyname);
						break;
					}
					case 0x0109: { //setv
						var_result = (uint8_t*)calloc(SyncHash::DEFAULT_SIZE, sizeof(uint8_t));
						if (getVarName(buffer, var_result) == -1 ||
							self->hTable->insert(keyname, var_result, self->ident) == -1) {
							send_err_header(header, buffer);
						} else {
							send_header(header, buffer);
						}
						free(var_result);
						free(keyname);
						break;
					}
					default:
						header.err_code = ENOTSUP;
						send_header(header, buffer);
				}
			}
		} else {
			switch(header.op) {
				case 0x0201: // read
				case 0x0202: // write
					if(conv_frm_nbo<uint16_t>(buffer, filename_size) == -1) {
						send_err_header(header, buffer);
						break;
					}
					filename = (uint8_t *)calloc(filename_size + 1, sizeof(uint8_t));
					buffer.getBytes(filename_size, filename);
					filename[filename_size] = '\0';
					if (conv_frm_nbo<uint64_t>(buffer, offset) == -1 || 
						conv_frm_nbo<uint16_t>(buffer, buff_size) == -1) {

						}
					if((header.op == 0x0201
							? read_file((char *)filename, offset, buff_size, buffer, header)
							: write_file((char *)filename, offset, buff_size, buffer, header))
						== -1) {
						send_err_header(header, buffer);
					}
					free(filename);
					break;
				case 0x0210: // create
					if(conv_frm_nbo<uint16_t>(buffer, filename_size) == -1) {
						send_err_header(header, buffer);
						break;
					}
					filename = (uint8_t *)calloc(filename_size + 1, sizeof(uint8_t));
					buffer.getBytes(filename_size, filename);
					filename[filename_size] = '\0';
					if(create((char *)filename) == -1) {
						send_err_header(header, buffer);
					} else {
						send_header(header, buffer);
					}
					free(filename);
					break;
				case 0x0220: // filesize
					if(conv_frm_nbo<uint16_t>(buffer, filename_size) == -1) {
						send_err_header(header, buffer);
						break;
					}
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
					if(conv_frm_nbo<uint16_t>(buffer, filename_size) == -1) {
						send_err_header(header, buffer);
						break;
					}
					filename = (uint8_t *)calloc(filename_size + 1, sizeof(uint8_t));
					buffer.getBytes(filename_size, filename);
					filename[filename_size] = '\0';
					if (self->hTable->dump((char*)filename, self->ident) == -1) {
						header.err_code = errno;
						send_header(header, buffer);
					} else {
						send_header(header, buffer);
					}
					free(filename);
					break;
				case 0x0302: //load
					if(conv_frm_nbo<uint16_t>(buffer, filename_size) == -1) {
						send_err_header(header, buffer);
						break;
					}
					filename = (uint8_t *)calloc(filename_size + 1, sizeof(uint8_t));
					buffer.getBytes(filename_size, filename);
					filename[filename_size] = '\0';
					if (self->hTable->load((char*)filename, self->ident) == -1) {
						header.err_code = errno;
						send_header(header, buffer);
					} else {
						send_header(header, buffer);
					}
					free(filename);
					break;
				default:
					header.err_code = ENOTSUP;
					send_header(header, buffer);
			}
		}
	}
}

#endif
