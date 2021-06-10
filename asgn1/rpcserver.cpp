/**
 * rpcserver
 * 
 * Description Pending
 * 
 * @author Perry David Ralston Jr.
 * @date 10/19/2020
 **/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <err.h>
#include <fcntl.h>
#include <unistd.h>
#include "bounded_buffer.h"

const int16_t MAX_HOSTNAME_SIZE = 1000;
const int16_t MIN_PORT_VAL = 1024;
struct resp_header {
    uint8_t req_ident[4];
    uint16_t op = 0;
    uint8_t err_code = 0;
};

template <class var_int_type>
void conv_to_nbo(Bounded_Buffer&, var_int_type);
template <class var_int_type>
var_int_type conv_frm_nbo(Bounded_Buffer&);
void build_header(resp_header&, Bounded_Buffer&);
void send_header(resp_header&, Bounded_Buffer&);
void send_math_response(resp_header&, Bounded_Buffer&, int64_t);
int8_t add(const int64_t&, const int64_t&, int64_t&);
int8_t subtract(const int64_t&, const int64_t&, int64_t&);
int8_t multiply(const int64_t&, const int64_t&, int64_t&);
int64_t read_file(char*, uint64_t, uint16_t, Bounded_Buffer&, resp_header&);
int64_t write_file(char*, uint64_t, uint16_t, Bounded_Buffer&);
int64_t create(char*);
int64_t filesize(char*);

int main(int argc, char const *argv[]) {
    if (argc != 2) {
        errx(EXIT_FAILURE, "Usage: ./rpcserver <host_name>:<port>");
    }
    char host_name[MAX_HOSTNAME_SIZE];
    uint16_t port = 0;
    //hostname is too large
    if(strstr(argv[1], ":") - argv[1] > MAX_HOSTNAME_SIZE) {
        errx(EXIT_FAILURE, "hostname too large");
    }
    if (sscanf(argv[1], "%[^:]:%hu", host_name,&port) < 2) {
        errx(EXIT_FAILURE, "Argument format <host_name>:<port>");
    }
    if (port < MIN_PORT_VAL) {
        errx(EXIT_FAILURE, "Port must be > %d", MIN_PORT_VAL);
    }
    //TODO check the below code for failures and handle accordingly
    /* Sourced from: https://canvas.ucsc.edu/courses/36179/pages/setting-up-sockets-for-basic-client-slash-server-stream-communication */
    struct hostent *hent = gethostbyname(host_name);
    struct sockaddr_in addr;
    memcpy(&addr.sin_addr.s_addr, hent->h_addr, hent->h_length);
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    int enable = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) err(EXIT_FAILURE, "%s\n", strerror(errno));
    listen(sock, 0);
    
    Bounded_Buffer buffer;
    resp_header header;
    int64_t arith_arg1, arith_arg2, arith_result, offset, file_size;
    uint16_t filename_size = 0, buff_size = 0;
    uint8_t* filename;
    while (true) {
        buffer.client_fd = accept(sock, NULL, NULL);
        if (buffer.client_fd == -1) {
            err(EXIT_FAILURE, "%s", strerror(errno));
        }
        while(buffer.client_fd != 0) {
            build_header(header, buffer);
            if (buffer.client_fd == 0) break;
            switch (header.op) {
                case 0x0101://add
                    arith_arg1 = conv_frm_nbo <uint64_t> (buffer);
                    arith_arg2 = conv_frm_nbo <uint64_t> (buffer);
                    if(buffer.client_fd != 0) {
                        if(add(arith_arg1, arith_arg2, arith_result) == -1) {
                            header.err_code = EINVAL;
                            send_header(header, buffer);
                        } else {
                            send_math_response(header, buffer, arith_result);
                        }
                    }
                    break;
                case 0x0102://sub
                    arith_arg1 = conv_frm_nbo <uint64_t> (buffer);
                    arith_arg2 = conv_frm_nbo <uint64_t> (buffer);
                    if(buffer.client_fd != 0) {
                        if(subtract(arith_arg1, arith_arg2, arith_result) == -1) {
                            header.err_code = EINVAL;
                            send_header(header, buffer);
                        } else {
                            send_math_response(header, buffer, arith_result);
                        }
                    }
                    break;
                case 0x0103://mult
                    arith_arg1 = conv_frm_nbo <uint64_t> (buffer);
                    arith_arg2 = conv_frm_nbo <uint64_t> (buffer);
                    if(buffer.client_fd != 0) {
                        if(multiply(arith_arg1, arith_arg2, arith_result) == -1) {
                            header.err_code = EINVAL;
                            send_header(header, buffer);
                        } else {
                            send_math_response(header, buffer, arith_result);
                        }
                    }
                    break;
                case 0x0201://read
                    filename_size = conv_frm_nbo <uint16_t> (buffer);
                    filename = (uint8_t*)calloc(filename_size + 1, sizeof(uint8_t));
                    buffer.getBytes(filename_size, filename);
                    filename[filename_size] = '\0';
                    offset = conv_frm_nbo <uint64_t> (buffer);
                    buff_size = conv_frm_nbo <uint16_t> (buffer);
                    if(read_file((char*)filename, offset, buff_size, buffer, header) == -1) {
                        header.err_code = errno;
                        send_header(header, buffer);
                    }
                    free(filename);
                    break;
                case 0x0202://write
                    filename_size = conv_frm_nbo <uint16_t> (buffer);
                    filename = (uint8_t*)calloc(filename_size + 1, sizeof(uint8_t));
                    buffer.getBytes(filename_size, filename);
                    filename[filename_size] = '\0';
                    offset = conv_frm_nbo <uint64_t> (buffer);
                    buff_size = conv_frm_nbo <uint16_t> (buffer);
                    if(write_file((char*)filename, offset, buff_size, buffer) == -1) {
                        header.err_code = errno;
                    }
                    send_header(header, buffer);
                    free(filename);
                    break;
                case 0x0210://create
                    filename_size = conv_frm_nbo <uint16_t> (buffer);
                    filename = (uint8_t*)calloc(filename_size + 1, sizeof(uint8_t));
                    buffer.getBytes(filename_size, filename);
                    filename[filename_size] = '\0';
                    if (create((char*)filename) == -1) {
                        header.err_code = errno;
                        send_header(header, buffer);
                    } else {
                        send_header(header, buffer);
                    }
                    free(filename);
                    break;
                case 0x0220://filesize
                    filename_size = conv_frm_nbo <uint16_t> (buffer);
                    filename = (uint8_t*)calloc(filename_size + 1, sizeof(uint8_t));
                    buffer.getBytes(filename_size, filename);
                    filename[filename_size] = '\0';
                    file_size = filesize((char*)filename);
                    if (file_size == -1) {
                        header.err_code = errno;
                        send_header(header,buffer);
                    } else {
                        send_header(header, buffer);
                        conv_to_nbo<int64_t>(buffer, file_size);
                        buffer.flush();
                    }
                    free(filename);
                    break;
                default:
                    header.err_code = EBADRQC;
                    send_header(header, buffer);
            }
        }
    }   
    return 0;
}

template <class var_int_type>
void conv_to_nbo(Bounded_Buffer& b_buff, var_int_type to_conv) {
    uint8_t to_push = 0;
    errno = 0;
    ssize_t size = sizeof(to_conv) - 1;
    for (ssize_t i = size; 0 <= i; --i) {
        to_push = (to_conv >> 8 * i) &0xFF;
        if (b_buff.pushByte(to_push) == -1) {
            //TODO error handling
        }
    }
}

template <class var_int_type>
var_int_type conv_frm_nbo(Bounded_Buffer& b_buff) {
    var_int_type ret_val = 0;
    for (size_t i = 0; i < sizeof(ret_val); ++i) {
        uint8_t* curr_byte = b_buff.getByte();
        if (curr_byte == NULL) {
            return -1;     
        }
        ret_val = (ret_val << 8) + *curr_byte;
    }
    return ret_val;
}

void build_header(resp_header& header, Bounded_Buffer& b_buff) {
    header.op = conv_frm_nbo <uint16_t> (b_buff);
    if (b_buff.client_fd == 0) return;
    b_buff.getBytes(sizeof(uint32_t), header.req_ident);
    if (b_buff.client_fd == 0) return;
    header.err_code = 0;
}

void send_header(resp_header& header, Bounded_Buffer& b_buff) {
    b_buff.clear();
    b_buff.pushBytes(sizeof(uint32_t), header.req_ident);
    if (b_buff.client_fd == 0) return;
    b_buff.pushByte(header.err_code);
    if (b_buff.client_fd == 0) return;
    b_buff.flush();
}

void send_math_response(resp_header& header, Bounded_Buffer& b_buff, int64_t data) {
    if (b_buff.client_fd == 0) return;
    send_header(header, b_buff);
    if (b_buff.client_fd == 0) return;
    conv_to_nbo(b_buff, data);
    b_buff.flush();
}

int8_t add(const int64_t& a, const int64_t& b, int64_t& result) {
    result = a + b;
    if (!((a ^ b) < 0) && (a ^ result) < 0) {
        return -1;
    }
    return 0;

}

int8_t subtract(const int64_t& a, const int64_t& b, int64_t& result) {
    result = a - b;
    if ((a ^ b) < 0 && (a ^ result) < 0){
        return -1;
    }
    return 0;
}

int8_t multiply(const int64_t& a, const int64_t& b, int64_t& result) {
    result = a * b;
    if (a != 0 && b != 0 && a != result / b) {
        return -1;
    }
    return 0;
}

int64_t read_file(char* filename, uint64_t offset, uint16_t bufsize, Bounded_Buffer& b_buff, resp_header& header) {
    int file_d = open(filename, O_RDONLY|O_EXCL);
    if (file_d == -1) {
        b_buff.dump(bufsize);
        return -1;
    }
    int64_t file_size = filesize(filename);
    if (file_size == -1) {
        close(file_d);
        return -1;
    }
    if ((int64_t)offset > file_size || (file_size - offset) < bufsize) {
        close(file_d);
        errno = EINVAL;
        return -1;
    }
    if (lseek(file_d, offset, SEEK_SET) == -1) {
        close(file_d);
        return -1;
    }
    send_header(header, b_buff);
    conv_to_nbo<uint16_t>(b_buff, bufsize);
    int64_t bytes_read = b_buff.from_file(file_d, bufsize);
    close(file_d);
    if (bytes_read == -1) return -1;
    return bytes_read;
}

int64_t write_file(char* filename, uint64_t offset, uint16_t bufsize, Bounded_Buffer& b_buff) {
    int file_d = open(filename, O_WRONLY|O_EXCL);
    if (file_d == -1) {
        b_buff.dump(bufsize);
        return -1;
    }
    if (lseek(file_d, offset, SEEK_SET) == -1) {
        close(file_d);
        return -1;
    }
    int64_t bytes_written = b_buff.to_file(file_d, bufsize);
    close(file_d);
    if (bytes_written == -1) return -1;
    return bytes_written;
}

int64_t create(char* filename) {
    int fd = open(filename, O_RDWR|O_CREAT|O_EXCL, S_IRWXU);
    if (fd == -1 || close(fd) == -1) return -1;
    return 0;
}

int64_t filesize(char* filename) {
    struct stat file_stats;
    if (stat(filename, &file_stats) == -1) {
        return -1;
    }
    return file_stats.st_size;
}

