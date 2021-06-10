/** Bounded_Buffer header file
 *  Bounded class to maintain a buffer for reading and writing
 *  client messages
 * 
 *  @author Perry David Ralston Jr.
 *  @date 10/28/2020 
 */

#ifndef BOUNDED_BUFFER
#define BOUNDED_BUFFER

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <err.h>
#include <unistd.h>

class Bounded_Buffer {
    private:
        static const ssize_t BUFFER_SIZE = 16000;
        uint8_t* root;
        uint8_t* start;
        uint8_t* end;
        void close_cl();
        ssize_t fill();
    public:
        Bounded_Buffer();
        ~Bounded_Buffer();
        bool isEmpty() {return start == end;}
        void clear() {start = end = root;}
        void dump(uint16_t);
        ssize_t to_file(int, ssize_t);
        ssize_t flush();
        ssize_t from_file(int, ssize_t);
        uint8_t* getByte();
        int8_t getBytes(size_t size, uint8_t* dest);
        int8_t pushByte(uint8_t in_byte);
        int8_t pushBytes(size_t size, uint8_t* in_bytes); 

        int client_fd;
};

#endif
