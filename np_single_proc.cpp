#include "pack.hpp"

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


VC clients;
int avail[30];
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


void broadcast(const char *msg, int len) {
	for (VIT it = clients.begin(); it != clients.end(); it++) {
		Write(it->fd, msg, len);
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
	msg = "*** Error: user #" + to_string(id) + " does not exist yet. ***\n";
	Write(it->fd, msg.c_str(), msg.size());
	return;

}
bool cmp(client a, client b) {
	return a.id < b.id;
}
VIT who(VIT it) { //done
	int id = it->id;
	VIT pin;
	sort(clients.begin(), clients.end(), cmp);
	for (VIT iter = clients.begin(); iter != clients.end(); iter++) {
		if (id == iter->id) {
			pin = iter;
			break;
		}
	}
	string banner = "<ID>\t<nickname>\t<IP:port>\t<indicate me>\n";
	Write(pin->fd, banner.c_str(), banner.size());
	for (VIT iter = clients.begin(); iter != clients.end(); iter++) {
		string msg;
		if (iter == pin) {
			msg = to_string(iter->id) + '\t' + iter->name + '\t' + iter->ip + ':' + to_string(iter->port) + '\t' + "<-me\n";
			//sz = sprintf(buf, "%d\t%s\t%s:%d\t<-me", it->id, it->name, it->ip, it->name);
		} else {
			msg = to_string(iter->id) + '\t' + iter->name + '\t' + iter->ip + ':' + to_string(iter->port) + "\n";
			//sz = sprintf(buf, "%d\t%s\t%s:%d\n", it->id, it->name, it->ip, it->name);
		}
		Write(pin->fd, msg.c_str(), msg.size());	
	}
	return pin;
}

void name(VIT it, string s) { //done
	string msg;
	for (VIT iter = clients.begin(); iter != clients.end(); iter++) {
		if (s == iter->name) {
			msg = "*** User \'" + s + "\' already exists. ***\n";	
			Write(it->fd, msg.c_str(), msg.size());
			return;
		}
	}
	msg = "*** User from "+ it->ip + ":" + to_string(it->port) +" is named \'" + s + "\'. ***\n";
	it->name = s;
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
	avail[it->id - 1] = 0;
	for (VIT iter = clients.begin(); iter != clients.end(); iter++) {
		if (iter->up.find(it->id) != iter->up.end()) {
			iter->up.erase(it->id);
		}
	}
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
	//cout << "parse\n";
	if (input.empty()) {
		Write(it->fd, "% ", 2);
		return;
	}
	res = parse(input);
	//res.print();
	cout << "Input: " << input << '\n';
	setenv("PATH", it->env.c_str(), 1);
	signal(SIGCHLD, SIG_IGN);
	it->rip++;
	//cout << "Done Parse\n";
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
	//if (res.writePipe) {
		//int user = 0;
		//for (VIT iter = clients.begin(); iter != clients.end(); iter++ ) {
			//if (res.writePipe == iter->id) {
				//user = 1;
				//if (iter->up.find(it->id) == iter->up.end()) {
					//int fd[2];
					//pipe(fd);
					//iter->up[it->id][0] = fd[0];
					//iter->up[it->id][1] = fd[1];
					////cout << "create " << it->id << " to " << iter->id << '\n';
					//string msg;
					//msg = "*** " + it->name + " (#" + to_string(it->id) + ") just piped \'" + input + "\' to " +  iter->name;
					//msg += " (#" + to_string(iter->id) + ") ***\n";
					//broadcast(msg.c_str(), msg.size());
					
				//} else {
					////pipe exists
					//string msg;
					//msg = "*** Error: the pipe #" + to_string(it->id) + "->#" + to_string(iter->id) + " already exists. ***\n";
					//Write(it->fd, msg.c_str(), msg.size());
					//res.writePipe = -1;
				//}
			//}
		//}
		//if (!user) {
			//string msg;
			//msg = "*** Error: user #" + to_string(res.writePipe) + " does not exist yet. ***\n";
			//Write(it->fd, msg.c_str(), msg.size());
		//}
	//} 
	if (res.readPipe) {
		int user = 0;
		for (VIT iter = clients.begin(); iter != clients.end(); iter++) {
			if (iter->id == res.readPipe) {
				user = 1;
				if (it->up.find(res.readPipe) == it->up.end()) {
					//pipe dont exist	
					string msg;
					msg = "*** Error: the pipe #" + to_string(iter->id) + "->#" + to_string(it->id) + " does not exist yet. ***\n";
					Write(it->fd, msg.c_str(), msg.size());
				} else {
					// recv pipe
					string msg;
					msg = "*** " + it->name + " (#" + to_string(it->id) + ") just received from " + iter->name + " (#" +to_string(iter->id) + ") by \'" + input + "\' ***\n";
					broadcast(msg.c_str(), msg.size());
					//cerr << "Reading -> so need to close write end "<< it->up[res.readPipe][1] << '\n';
					//for (VIT tmp = clients.begin(); tmp != clients.end(); tmp++) {
						//cerr << "id = " << tmp->id << " fd = " << tmp->fd << '\n';
					//}
					close(it->up[res.readPipe][1]);
				}
				break;
			}
		}
		if (!user) {
			//user dont exist
			string msg;
			msg = "*** Error: user #" + to_string(res.readPipe) + " does not exist yet. ***\n";
			Write(it->fd, msg.c_str(), msg.size());
		}
	}
	if (res.writePipe) {

		int user = 0;
		for (VIT iter = clients.begin(); iter != clients.end(); iter++ ) {
			if (res.writePipe == iter->id) {
				user = 1;
				if (iter->up.find(it->id) == iter->up.end()) {
					int fd[2];
					pipe(fd);
					//cerr << "Opening: ";
					//cerr << fd[0] << ' ' << fd[1] << '\n';
					iter->up[it->id][0] = fd[0];
					iter->up[it->id][1] = fd[1];
					//cout << "create " << it->id << " to " << iter->id << '\n';
					string msg;
					msg = "*** " + it->name + " (#" + to_string(it->id) + ") just piped \'" + input + "\' to " +  iter->name;
					msg += " (#" + to_string(iter->id) + ") ***\n";
					broadcast(msg.c_str(), msg.size());
				} else {
					//pipe exists
					string msg;
					msg = "*** Error: the pipe #" + to_string(it->id) + "->#" + to_string(iter->id) + " already exists. ***\n";
					Write(it->fd, msg.c_str(), msg.size());
					res.writePipe = -1;
				}
			}
		}
		if (!user) {
			string msg;
			msg = "*** Error: user #" + to_string(res.writePipe) + " does not exist yet. ***\n";
			Write(it->fd, msg.c_str(), msg.size());
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
		if ( !strncmp(argv[0], "printenv", 8) ) {
			if (cnt < 2) {
				string msg = "Command error\n";
				Write(it->fd, msg.c_str(), msg.size());
				continue;
			}
			if ( getenv(argv[1]) != NULL) { 
				string msg = getenv(argv[1]);
				msg += '\n';
				Write (it->fd, msg.c_str(), msg.size());
			}
			continue;
		} else if ( !strncmp(argv[0], "setenv", 6) ){
			if (cnt < 3) {
				string msg = "Command error\n";
				Write(it->fd, msg.c_str(), msg.size());
				continue;
			}
			it->env = argv[2];
			continue;
		} else if ( !strncmp(argv[0], "name", 4) ) {
			name(it, argv[1]);	
			continue;
		} else if ( !strncmp(argv[0], "yell", 4) ) {
			string msg;
			int found = input.find(argv[1]);
			msg = input.substr(found);
			yell(it, msg);
			continue;
		} else if ( !strncmp(argv[0], "tell", 4) ) {
			string msg;
			int found = input.find(argv[2]);
			msg = input.substr(found);
			tell(it, atoi(argv[1]), msg);
			continue;
		} else if ( !strncmp(argv[0], "who", 3) ) {
			VIT pin = who(it);
			it = pin;
			continue;
		} 
		if (i != res.cmd.size()-1) {
			cur = new int[2];
			while ( pipe(cur) );
		}

		int pid;

		while ( (pid = fork()) == -1) ;

		lastpid = pid;

		if (it->mp.find(it->rip) != it->mp.end()) {
			//cerr << it->mp[it->rip][1] << '\n';
			close(it->mp[it->rip][1]);
		}
		if (pid == 0) {
			if (res.cmd.size() != 1) {
			// multiple subprocess
				if (i == 0) {
					// first process
					if (it->mp.find(it->rip) != it->mp.end()) {
						dup2(it->mp[it->rip][0], STDIN_FILENO);
					} else if ( res.readPipe ) {
						int user = 0, exist = 0;
						VIT iter;
						for (iter = clients.begin(); iter != clients.end(); iter++) {
							if (iter->id == res.readPipe) {
								user = 1;
								break;
							}
						}
						if (!user) {

							int d = open("/dev/null", O_RDWR);
							dup2(d, STDIN_FILENO);
						} else if (it->up.find(res.readPipe) != it->up.end()) {
							dup2(it->up[res.readPipe][0], STDIN_FILENO);

							//it->up.erase(res.readPipe);
						} else {
							// user pipe dont exist

							int d = open("/dev/null", O_RDWR);
							dup2(d, STDIN_FILENO);
						}

					}
					dup2(cur[1], STDOUT_FILENO);
					//cerr << cur[0] << ' ' << cur[1] << '\n';
					close(cur[0]);
					close(cur[1]);
				} else if (i == res.cmd.size()-1) {
					// last process
					dup2(prev[0], STDIN_FILENO);
					//cerr << prev[0] << ' ' << prev[1] << '\n';
					close(prev[0]);
					close(prev[1]);

					if (res.np) {
						dup2(it->mp[it->rip + res.np][1], STDOUT_FILENO);
					} else if (res.exp) {
						dup2(it->mp[it->rip + res.exp][1], STDOUT_FILENO);
						dup2(it->mp[it->rip + res.exp][1], STDERR_FILENO);
					} else if (res.filename != "") {
						int f;
						f = open(res.filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
						dup2(f, STDOUT_FILENO);
						close(f);
					} else if (res.writePipe) {
						int user = 0;
						
						VIT iter;
						for (iter = clients.begin(); iter != clients.end(); iter++) {
							if (iter->id == res.writePipe) {
								user = 1;	
								if (iter->up.find(it->id) != iter->up.end()) {
									
									dup2(iter->up[it->id][1], STDOUT_FILENO);
									//cerr << iter->up[it->id][1] << '\n';
									close(iter->up[it->id][1]);
									break;
								} 
							}
						}
						if (res.writePipe == -1) {
							int d = open("/dev/null", O_RDWR);
							dup2(d, STDOUT_FILENO);
								
						} else if (!user) {

							int d = open("/dev/null", O_RDWR);
							dup2(d, STDOUT_FILENO);
							//no user exist
						} 
					} else {
						dup2(it->fd, STDOUT_FILENO);
						dup2(it->fd, STDERR_FILENO);
						//?
						//close(it->fd);
					}
				} else {
					dup2(prev[0], STDIN_FILENO);
					close(prev[0]);
					close(prev[1]);
					dup2(cur[1], STDOUT_FILENO);
					close(cur[0]);
					close(cur[1]);
					dup2(it->fd, STDERR_FILENO);
				}
			} else {
				if (it->mp.find(it->rip) != it->mp.end()) {
					dup2(it->mp[it->rip][0], STDIN_FILENO);
				} else if ( res.readPipe ) {
					int user = 0, exist = 0;
					VIT iter;
					for (iter = clients.begin(); iter != clients.end(); iter++) {
						if (iter->id == res.readPipe) {
							user = 1;
							break;
						}
					}
					if (!user) {
						int d = open("/dev/null", O_RDWR);
						dup2(d, STDIN_FILENO);
					} else if (it->up.find(res.readPipe) != it->up.end()) {
						dup2(it->up[res.readPipe][0], STDIN_FILENO);
						it->up.erase(res.readPipe);
					} else {
						// user pipe dont exist
						int d = open("/dev/null", O_RDWR);
						dup2(d, STDIN_FILENO);
					}
				
				}
				if (res.np) {
					dup2(it->mp[it->rip + res.np][1], STDOUT_FILENO);
				} else if (res.exp) {
					dup2(it->mp[it->rip + res.exp][1], STDOUT_FILENO);
					dup2(it->mp[it->rip + res.exp][1], STDERR_FILENO);
				} else if (res.filename != "") {
					int f;
					f = open(res.filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
					if ( dup2(f, STDOUT_FILENO)  == -1) {
						perror("dup2 error");
					}
					close(f);
				} else if (res.writePipe) {
					int user = 0, exist = 0;
					
					VIT iter;
				
					for (iter = clients.begin(); iter != clients.end(); iter++) {
						if (iter->id == res.writePipe) {
							user = 1;	
							if (iter->up.find(it->id) != iter->up.end()) {
								dup2(iter->up[it->id][1], STDOUT_FILENO);
								//close(iter->up[it->id][1]);

								break;
							} 
						}
					}
					if (!user) {
						int d = open("/dev/null", O_RDWR);
						dup2(d, STDOUT_FILENO);
						//no user exist
					}
		
			
				} else {
					dup2(it->fd, STDOUT_FILENO);
					//dup2(it->fd, STDERR_FILENO);
					// ??
					//close(it->fd);
				}
			}
			dup2(it->fd, STDERR_FILENO);
			//cout << it->fd << '\n';
			if (execvp(argv[0], argv) < 0 ) {
					// poss
				string msg; 
				msg = "Unknown command: [" + string(argv[0]) + "].\n";
				Write(it->fd, msg.c_str(), msg.size());

				exit(-1);
			}
		} else {
			if (prev) {
				//cerr << prev[0] << ' ' << prev[1] << '\n';
				close(prev[0]);
				close(prev[1]);
				delete[] prev;
			}
			prev = cur;
		}

	}
	int status;
    if ( res.np == 0 && res.exp == 0 && res.writePipe == 0) {
    	waitpid(lastpid, nullptr, WUNTRACED);
    }
    if (it->mp.find(it->rip) != it->mp.end()) {
    	//close(it->mp[it->rip][1]);
    	close(it->mp[it->rip][0]);
    	it->mp.erase(it->rip);
    }
	if (res.readPipe) {
		if (it->up.find(res.readPipe) != it->up.end()) {
			//cerr << "Finish reading so need to close pipe: ";
			//cerr << it->up[res.readPipe][0] << ' ' << it->up[res.readPipe][1] << '\n';
			close(it->up[res.readPipe][0]);
		//	close(it->up[res.readPipe][1]);
			it->up.erase(res.readPipe);
		}
	}
	Write(it->fd, "% ", 2);
	
}



int main (int argc, char** argv) {
	int ms, nready, ns;
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
	memset(avail, 0, sizeof(avail));
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
			int id = 0;
			for (int i = 0; i < 30; i++) {
				if (avail[i] == 0) {
					avail[i] = 1;
					id = i+1;
					break;
				}
			}
			cout << "Accept id = " << id << " fd = "<< ns << '\n';
			clients.pb(client(ns, "(no name)", string(ip), env, port, id));
			
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
