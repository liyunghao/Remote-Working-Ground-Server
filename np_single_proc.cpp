#include "pack.hpp"

class client {
public:
	client (int _fd, string _name, string _ip, string _env, int _port, int _id): fd(_fd), name(_name), ip(_ip), env(_env), port(_port), id(_id) {}
	
	int fd, port, id;
	string name, ip;
	string env;
};


VC clients;
map<int, int> id2fd;
int assign;
map<int, int[2]> up;

parseRes parse (string input) {
	int begin = 0;
	parseRes res;
	int f = 1;
	for (int i = 0; i < input.size(); i++) {
		switch(input[i]) {
			case '|':
				if (i == input.size() - 1) {
					perror("Command Error\n");
					exit(-1);
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
					perror("Command Error\n");
					exit(-1);
				} 
				if (begin < i )
					res.cmd.pb(input.substr(begin, i-begin-1 ));
				res.exp = atoi(input.substr(i+1).c_str());
				f = 0;
				break;
			
			case '>':
				if (i == input.size() - 1) {
					perror("Command Error\n");
					exit(-1);
				} 
				if ( input[i+1] == ' ' ) {
					if (begin < i )
						res.cmd.pb(input.substr(begin, i-begin-1));
					res.filename = input.substr(i+2);
					f = 0;
				} else {
					if (begin < i )
						res.cmd.pb(input.substr(begin, i-begin-1));
					int j;
					for (j = i+1; j < input.size(); j++) {
						if ( input[j] == ' ' ) 
							break;	
					}	
					//cout << i << ' ' << j << '\n';
					//cout <<  atoi(input.substr(i+1, j-i).c_str()) << '\n';
					res.writePipe = atoi(input.substr(i+1, j-i).c_str());
					i = j;
					begin = i+1;
				}
				break;
			
			case '<':
				if (i == input.size() - 1) {
					perror("Command Error\n");
					exit(-1);
				} 
				if ( input[i+1] == ' ' ) {
					perror("Command Error\n");
					exit(-1);
				} else {
					if (begin < i )
						res.cmd.pb(input.substr(begin, i-begin-1));
					int j;
					for (j = i+1; j < input.size(); j++) {
						if ( input[j] == ' ')
							break;	
					}	
					//cout << i << ' ' << j << '\n';
					//cout <<  atoi(input.substr(i+1, j-i).c_str()) << '\n';
					res.readPipe = atoi(input.substr(i+1, j-i).c_str());
					i = j;
					begin = i+1;
					//cout << begin << '\n';
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


void broadcast(char *msg, int len) {
	for (VIT it = clients.begin(); it != clients.end(); it++) {
		int sock = it->fd;
		Write(sock, msg, len);
	}
	return;
}

void yell(VIT it, string msg) {
	char buf[MSGLEN];
	memset(buf, 0, sizeof(buf));
	int sz;
	sz = sprintf(buf, "*** %s yelled ***: %s\n", it->name, msg);
	broadcast(buf, sz);
}

void tell(VIT it, int id, string msg) {
	char buf[MSGLEN];
	memset(buf, 0, sizeof(buf));
	int sz;
	if ( id2fd.find(id) != id2fd.end() ) {
		sz = sprintf(buf, "*** %s told you ***: %s\n", it->name, msg);
		Write(id2fd[id], buf, sz);
		return;
	}
	//write error
	sz = sprintf(buf, "*** ERROR: user #%d does not exist yet. ***\n", id);
	Write(it->fd, buf, sz);
	return;

}

void who(VIT x) {
	//sort(clients);
	string banner = "<ID>\t<nickname>\t<IP:port>\t<indicate me>\n";
	for (VIT it = clients.begin(); it != clients.end(); it++) {
		char buf[MSGLEN];
		memset(buf, 0, sizeof(buf));
		int sz;
		if (it == x) {
			sz = sprintf(buf, "%d\t%s\t%s:%d\t<-me", it->id, it->name, it->ip, it->name);
		} else {
			sz = sprintf(buf, "%d\t%s\t%s:%d\n", it->id, it->name, it->ip, it->name);
		}
		write(x->fd, buf, sz);	
	}
	return;
}

void name(VIT it, string s) {
	char buf[MSGLEN];
	memset(buf, 0, sizeof(buf));
	int sz;
	for (VIT it = clients.begin(); it != clients.end(); it++) {
		if (s == it->name) {
			sz = sprintf(buf, "*** User \'%s\' already exists.\n", s);	
			Write(it->fd, buf, sz);
			return;
		}
	}
	sz = sprintf(buf, "*** User from %s:%d is named \'%s\'\n", it->ip, it->port, s);
	broadcast(buf, sz);
	return;

}

void login(VIT it) {
	char buf[MSGLEN];
	memset(buf, 0, sizeof(buf));
	int sz;
	sz = sprintf(buf, "*** User \'%s\' entered from %s:%d. ***\n", it->name, it->ip, it->port);
	broadcast(buf, sz);
}

void logout(VIT it) {
	char buf[MSGLEN];
	memset(buf, 0, sizeof(buf));
	int sz;

	sz = sprintf(buf, "*** User \'%s\' left. ***\n", it->name);
	//clear info
	close(it->fd);
	assign = assign > it->id ? it->id : assign;
	
	clients.erase(it);
	broadcast(buf, sz);
}

void welcome(int fd) { //done
	char buf[MSGLEN];
	memset(buf, 0, sizeof(buf));
	int sz;
	sz = sprintf(buf, "****************************************\n** Welcome to the information server. **\n****************************************\n");
	Write(fd, buf, sz);
	return;

}

void exec_cmd(string input) {
	parseRes res;
	res = parse(input);
	res.print();
	
}



int main (int argc, char** argv) {
	int ms, nready, ns;
	assign = 0;
	unsigned int clilen;
	char buf[MAXLINE];

	fd_set rfds, afds;
	sockaddr_in cliaddr, servaddr;
	unsigned int serv_port; 
	if (argc == 2) 
		serv_port = atoi(argv[1]);
	else {
		cout << "[usage] ./np_single_proc <port number>\n";
		exit(-1);
	}
	
	if ( (ms = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		perror("Socket error: ");		
		exit(-1);
	}
	servaddr.sin_family 	 = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port 	 	 = htons(serv_port);
 	int reuse = 1;
    setsockopt(ms, SOL_SOCKET, SO_REUSEADDR, (const char*) &reuse, sizeof(int));

	if ( bind(ms, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ) {
		perror("Bind error: ");
	}
	
	if ( (listen(ms, 30)) < 0 ) {
		perror("Listen error: ");
	}
	int nfds = getdtablesize();
	FD_ZERO(&afds);
	FD_SET(ms, &afds);
	cout << "here\n";
	while (1) {
		memcpy(&rfds, &afds, sizeof(rfds));

		if ( ( nready = select(nfds, &rfds, (fd_set *)0, (fd_set *)0, (struct timeval*)0 ) ) < 0 ) {
			perror("Select error: ");
		}
		
		if ( FD_ISSET(ms, &rfds) ) {
			clilen = sizeof(cliaddr);
			if ( ( ns = accept(ms, (struct sockaddr*) &cliaddr, &clilen) ) < 0 ) {
				perror("Accept error: ");
				exit(-1);
			}
			
			char ip[1024];
			inet_ntop(AF_INET, &cliaddr.sin_addr, ip, 1024);
			int port = ntohs(cliaddr.sin_port);
			clients.pb(client(ns, "unknown", string(ip), port, ++assign));
			cout << "new\n";	
			// welcome mes
			// broadcast
			welcome(ns);
			FD_SET(ns, &afds);
			if (--nready == 0)
				continue;
		}
		for (VIT it = clients.begin(); it != clients.end(); it++) {
			if (FD_ISSET(it->fd, &rfds)) {
				int sz;
				if ( (sz = Read(it->fd, buf, MAXLINE)) == 0) {
					FD_CLR(it->fd, &afds);
					//logout();
					it--;
				} else {
					//string cmd = line;
					//Write(it->fd, buf, sz);
					char cmd[MAXLINE];
					memset(cmd, 0, sizeof(cmd));

					for (int i = 0; i < sz; i++) {
						if ( int(buf[i]) != 13 && int(buf[i]) != 10 ) {
							cmd[i] = buf[i];
						} else {
							sz = i;
							break;
						}
					}
					string input = string(cmd);
					//cout << input << ' ' << input.size() << '\n';
					exec_cmd(input);
				}
	
			}
			if (--nready == 0) 
				break;
		}
	}

	

}
