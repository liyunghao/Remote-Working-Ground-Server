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
#include <utility>
#include <sys/ipc.h>
#include <sys/shm.h>

#define pb(x) push_back(x)
#define MAXLINE 16000
#define MAX_CLI 30
#define MSGLEN 1050
#define VIT vector<client>::iterator
#define VC vector<client>
#define PII pair<int, int>
using namespace std;



struct clientInfo {
	int pid, port, id;
	char name[25], ip[50];
	
	info(int _pid, int _port, int _id, char *name, char* ip):pid(_pid), port(_port), id(_id) {
		memset(name, 0, sizeof(name));
		memset(ip, 0, sizeof(ip));
		strcpy(name, _name);
		strcpy(ip, _ip);
	}
	bool operator >(const clientInfo &a) {
		return id > a.id;
	}
	

};

struct memory {
	struct clientInfo info[30];
	int avail[30];
	int userNum;
	char op[20], msg[1050];
};

class client {
public:
    client (int _fd, string _name, string _ip, string _env, int _port, int _id): fd(_fd), name(_name), ip(_ip), env(_env), port(_port), id(_id) {
        rip = 0;
    }

    int fd, port, id, rip;
    map<int, int[2]> mp;
    map<int, int[2]> up;
    string name, ip;
    string env;
};


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
parseRes parse (string input) {
	int begin = 0;
	parseRes res;
	int f = 1;
	for (int i = 0; i < input.size(); i++) {
		switch(input[i]) {
			case '|':
				if (i == input.size() - 1) {
						break;
				} else if ( input[i+1] == ' ') {
						if (begin < i)
								res.cmd.pb(input.substr(begin, i - begin - 1 ));
						begin = i+2;
				} else {
						if (begin < i )
								res.cmd.pb(input.substr(begin, i - begin - 1 ));
						res.np = atoi(input.substr(i+1).c_str());
						f = 0;
				}

				break;
			case '!':
				if (i == input.size() - 1) {
						break;
				}
				if (begin < i )
						res.cmd.pb(input.substr(begin, i-begin-1 ));
				res.exp = atoi(input.substr(i+1).c_str());
				f = 0;
				break;

			case '>':
				if (i == input.size() - 1) {
						break;
				}
				if ( input[i+1] == ' ' ) {
						if (begin < i )
								res.cmd.pb(input.substr(begin, i-begin-1));
						res.filename = input.substr(i+2);
						f = 0;
				} else {
						if (begin < i )
								res.cmd.pb(input.substr(begin, i-begin-1));
						int j = i+1;
						for ( ; j < input.size(); j++) {

								if ( input[j] == ' ' )
										break;
						}
						//cout << i << ' ' << j << '\n';
						//cout <<  atoi(input.substr(i+1, j-i).c_str()) << '\n';
						res.writePipe = atoi(input.substr(i+1, j-i).c_str());

						i = j;
						begin = i+1;
						if (begin > input.size()) {
								f = 0;
						}
				}
				break;

			case '<':
				if (i == input.size() - 1) {
						break;
				}
				if ( input[i+1] == ' ' ) {
						break;
				} else {
						if (begin < i )
								res.cmd.pb(input.substr(begin, i-begin-1));
						int j = i+1;
						for ( ; j < input.size(); j++) {
								if ( input[j] == ' ')
										break;
						}
						//cout << i << ' ' << j << '\n';
						//cout <<  atoi(input.substr(i+1, j-i).c_str()) << '\n';
						res.readPipe = atoi(input.substr(i+1, j-i).c_str());
						i = j;
						begin = i+1;
						//cout << begin << '\n';
						if (begin > input.size()) {
								f = 0;
						}
				}
				break;

			default:
				continue;
		}
	}
	if (f) {
			res.cmd.pb(input.substr(begin));
	}
	return res;
}

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

void Write(int fd, const void *ptr, size_t nbytes) {
	if (writen(fd, ptr, nbytes) != nbytes){
		perror("Write Error");
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
