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

void test_func(int sock);


int main() {
//init network
	/* Sourced from:
	 * https://canvas.ucsc.edu/courses/36179/pages/setting-up-sockets-for-basic-client-slash-server-stream-communication
	 */
	struct hostent *hent = gethostbyname("localhost");
	struct sockaddr_in addr;
	memcpy(&addr.sin_addr.s_addr, hent->h_addr, hent->h_length);
	addr.sin_port = htons(8912);
	addr.sin_family = AF_INET;
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	connect(sock, (struct sockaddr *)&addr, sizeof(addr));

	//sock is now connected

	return 0;
}