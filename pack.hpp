#pragma once
#include <iostream>
#include <unistd.h>
#include <vector>
#include <sstream>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <map>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <algorithm>
#define pb(x) push_back(x)
#define SERV_PORT 7001
#define MAXLINE 16000
#define MAX_CLI 10
#define MSGLEN 1050
#define VIT vector<client>::iterator
#define VC vector<client>
using namespace std;

struct parseRes {
	vector<string> cmd;
	int np, exp;
	string filename;
	int readPipe, writePipe;
	parseRes () {
		np = 0, exp = 0, readPipe = 0, writePipe = 0;
		
	}	
	void print() {
		cout << "Cmd\n--------------------------------\n";
		for (auto x : cmd) {
			cout << x << '\n';
		}
		cout << "Np : " << np << '\n'; 
		cout << "Exp : " << exp << '\n'; 
		cout << "Filename : " << filename << '\n';
		cout << "Readpipe : " << readPipe << '\n';
		cout << "Writepipe : " << writePipe << '\n';
		return;
	}
};

ssize_t Read(int fd, void *vptr, size_t maxlen) {
	ssize_t	n = read(fd, vptr, maxlen);
	if(n < 0){
			cerr << "Read Error" << endl;
			exit(EXIT_FAILURE);
	}
	char *ptr = (char * )vptr;
	ptr[n] = 0;
	return n;
}

ssize_t	writen(int fd, const void *vptr, size_t n) {
	size_t		nleft;
	ssize_t		nwritten;
	const char	*ptr;
	ptr = (char*)vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
			if (nwritten < 0 && errno == EINTR)
				nwritten = 0;
			else
				return(-1);
		}
		nleft -= nwritten;
		ptr   += nwritten;
	}
	return(n);
}

void Write(int fd, void *ptr, size_t nbytes) {
	if (writen(fd, ptr, nbytes) != nbytes){
		cerr << "Write Error" << endl;
		exit(EXIT_FAILURE);
	}
}
int readline(int fd, char *ptr, int maxlen) {
	int	n, rc;
	char	c;

	for (n = 1; n < maxlen; n++) {
		if ( (rc = read(fd, &c, 1)) == 1) {
			*ptr++ = c;
			if (c == '\n')
				break;
		} else if (rc == 0) {
			if (n == 1)
				return(0);	/* EOF, no data read */
			else
				break;		/* EOF, some data was read */
		} else
			return(-1);	/* error */
		}

		*ptr = 0;
		return(n);
}
