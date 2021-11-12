#include "pack.hpp"

class client {
public:
	client (int _fd, string _name, string _ip, string _env, int _port, int _id): fd(_fd), name(_name), ip(_ip), env(_env), port(_port), id(_id) {
		rip = 0;
	}
	
	int fd, port, id, rip;
	map<int, int[2]> mp;
	string name, ip;
	string env;
};


VC clients;
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


void broadcast(const char *msg, int len) {
	for (VIT it = clients.begin(); it != clients.end(); it++) {
		int sock = it->fd;
		Write(sock, msg, len);
	}
	return;
}

void yell(VIT it, string s) {
	string msg;
	msg = "*** " + it->name + " yelled ***: " + s + "\n";
	broadcast(msg.c_str(), msg.size());
}

void tell(VIT it, int id, string s) { //done
	string msg;
	for (VIT iter = clients.begin(); iter != clients.end(); iter++) {
		if (iter->id == id) {
			msg = "*** " + it->name + " told you ***: " + s + "\n";
			Write(iter->fd, msg.c_str(), msg.size());
			return;
		}
	}
	msg = "*** ERROR: user #" + to_string(id) + " does not exist yet. ***\n";
	Write(it->fd, msg.c_str(), msg.size());
	return;

}
bool cmp(client a, client b) {
	return a.id < b.id;
}
void who(VIT x, VIT it) { //done
	sort(clients.begin(), clients.end(), cmp);
	string banner = "<ID>\t<nickname>\t<IP:port>\t<indicate me>\n";
	Write(it->fd, banner.c_str(), banner.size());
	for (VIT it = clients.begin(); it != clients.end(); it++) {
		string msg;
		if (it == x) {
			msg = to_string(it->id) + '\t' + it->name + '\t' + it->ip + ':' + to_string(it->port) + '\t' + "<-me\n";
			//sz = sprintf(buf, "%d\t%s\t%s:%d\t<-me", it->id, it->name, it->ip, it->name);
		} else {
			msg = to_string(it->id) + '\t' + it->name + '\t' + it->ip + ':' + to_string(it->port) + '\t' + "\n";
			//sz = sprintf(buf, "%d\t%s\t%s:%d\n", it->id, it->name, it->ip, it->name);
		}
		write(it->fd, msg.c_str(), msg.size());	
	}
	return;
}

void name(VIT it, string s) { //done
	string msg;
	for (VIT it = clients.begin(); it != clients.end(); it++) {
		if (s == it->name) {
			msg = "*** User \'" + s + "\' already exists.\n";	
			Write(it->fd, msg.c_str(), msg.size());
			return;
		}
	}
	msg = "*** User from "+ it->ip + ":" + to_string(it->port) +" is named \'" + s + "\'\n";
	broadcast(msg.c_str(), msg.size());
	return;

}

void login(VIT it) { //done
	string msg;
	msg = "*** User \'" + it->name + "\' entered from " + it->ip + ":" + to_string(it->port) + ". ***\n";	
	broadcast(msg.c_str(), msg.size());
}

void logout(VIT it) { //done
	string msg;
	msg = "*** User \'" + it->name + "\' left. ***\n";
	close(it->fd);
	assign = assign > it->id ? it->id : assign;
	
	clients.erase(it);
	broadcast(msg.c_str(), msg.size());
}

void welcome(int fd) { //done
	char buf[MSGLEN];
	memset(buf, 0, sizeof(buf));
	int sz;
	sz = sprintf(buf, "****************************************\n** Welcome to the information server. **\n****************************************\n");
	Write(fd, buf, sz);
	return;

}
void sigHandler(int signo) {
	pid_t pid;
	int stat;
	while((pid = waitpid(-1, &stat, WNOHANG) > 0 ));
	return;
}

void exec_cmd(string input, VIT it) {
	parseRes res;
	res = parse(input);
	res.print();
	if (res.np) {
		if (it->mp.find(it->rip + res.np) == it->mp.end()) { 
			int fd[2];
			pipe(fd);
			it->mp[it->rip + res.np][0] = fd[0];
			it->mp[it->rip + res.np][1] = fd[1];
		}
	}
	if (res.exp) {
		if (it->mp.find(it->rip + res.exp) == it->mp.end()) {
			int fd[2];
			pipe(fd);
			it->mp[it->rip + res.exp][0] = fd[0];
			it->mp[it->rip + res.exp][1] = fd[1];	
		}
	}
	int cmdlen = res.cmd.size();
	int pipesz = cmdlen - 1;
	
	int lastpid = 0;
	int *prev = nullptr, *cur = nullptr;
	for (int i = 0; i < cmdlen; i++) {
		char *c = strdup(res.cmd[i].c_str());
		char *tok = strtok(c, " ");
		char *argv[256];
		int cnt = 0;
		while (tok) {
			argv[cnt++] = tok;
			tok = strtok(NULL, " ");
		}
		argv[cnt] = NULL;

	
	
	
	}
	
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
	//cout << "here\n";
	while (1) {
		rfds = afds;
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
			string env = "bin:.";
			clients.pb(client(ns, "(no name)", string(ip), env, port, ++assign));
			
			VIT cur = clients.begin() + clients.size() - 1;
			welcome(ns);
			login(cur);
			Write(ns, "% ", 2);

			FD_SET(ns, &afds);
			if (--nready == 0)
				continue;
		}
		for (VIT it = clients.begin(); it != clients.end(); it++) {
			if (FD_ISSET(it->fd, &rfds)) {
				//cout << it->name << '\n';
				int sz;
				if ( (sz = Read(it->fd, buf, MAXLINE)) == 0) {
					//FD_CLR(it->fd, &afds);
					//logout();
					//it--;
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
					if (input == "exit") {
						FD_CLR(it->fd, &afds);
						logout(it);
						it--;
						continue;
					} 
					
					exec_cmd(input, it);
				}		
				if (--nready == 0) 
					break;
			}

		}
	}

	

}
