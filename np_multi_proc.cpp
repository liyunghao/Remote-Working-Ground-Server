#include "pack.hpp"

int cliNum;
int rip;
map<int, int[2]> mp;
int shmid;
int curfd;

struct memory *shmptr;

void sigHandle(int signo) {
	//clear shm
	shmdt((void*)shmptr);
	shmctl(shmid, IPC_RMID, NULL);
	cout << "Parent exit by SIGINT\n";
	exit(0);
}
void childINT(int signo) {
	shmdt((void*)shmptr);
	cout << "Child exit by SIGINT\n";
	exit(0);
}
void recvUSR1(int signo) {
	// recv message
	string msg = shmptr->msg;
	Write(curfd, msg.c_str(), msg.size());
	
}
void recvUSR2(int signo) {
	//recv fifo req
}
void sigChild(int signo) {
	pid_t pid;
	int stat;
	while((pid = waitpid(-1, &stat, WNOHANG) > 0 ));
	return;
}
void broadcast(string msg) {
	memset(shmptr->msg, 0, sizeof(shmptr->msg));
	strcpy(shmptr->msg, msg.c_str());
	for (int i = 0; i < 30; i++) {
		if (shmptr->avail[i] == 1) {
			kill(shmptr->info[i].pid, SIGUSR1);
		}
	}
}
void name(int idx, string s) {
	string msg;
	for (int i = 0; i < 30; i++) {
		string tmp = shmptr->info[i].name;
		if (shmptr->avail[i] == 1 && tmp == s) {
			msg = "*** User \'" + s + "\' already exists. ***\n";
        	Write(curfd, msg.c_str(), msg.size());
            return;
		}
	}
	strcpy(shmptr->info[idx].name, s.c_str());
	msg = "*** User from "+ string(shmptr->info[idx].ip) + ":" + to_string(shmptr->info[idx].port) +" is named \'" + s + "\'. ***\n";
	broadcast(msg);
}

void yell(int idx, string s) {
	string msg;
	msg = "*** " + string(shmptr->info[idx].name) + " yelled ***: " + s + "\n";
	broadcast(msg);
}
void tell(int idx, int to, string s) {
	string msg;
	for (int i = 0; i < 30; i++) {
		if (shmptr->avail[i] == 1 && i == to) {
			msg = "*** " + string(shmptr->info[i].name) + " told you ***: " + s + "\n";
			memset(shmptr->msg, 0, sizeof(shmptr->msg));
			strcpy(shmptr->msg, msg.c_str());
			kill(shmptr->info[i].pid, SIGUSR1);
			return;
		}
	}
	msg = "*** Error: user #" + to_string(to) + " does not exist yet. ***\n";
	Write(curfd, msg.c_str(), msg.size());
	return;
}
void who(int idx) {
	string banner = "<ID>\t<nickname>\t<IP:port>\t<indicate me>\n";
	Write(curfd, banner.c_str(), banner.size());
	for (int i = 0; i < 30; i++) {
		if (shmptr->avail[i] == 0)
			continue;
		string msg;
		if (i == idx) {
			msg = to_string(shmptr->info[i].id) + '\t' + shmptr->info[i].name + '\t' + shmptr->info[i].ip + ':' + to_string(shmptr->info[i].port) + '\t' + "<-me\n";
		} else {
			msg = to_string(shmptr->info[i].id) + '\t' + shmptr->info[i].name + '\t' + shmptr->info[i].ip + ':' + to_string(shmptr->info[i].port) + "\n";
		}
		Write(curfd, msg.c_str(), msg.size());
	}
}

void login(int idx) {
	cout << "login\n";
	string msg;
	msg = "*** User \'" + string(shmptr->info[idx].name) + "\' entered from " + string(shmptr->info[idx].ip) + ":" + to_string(shmptr->info[idx].port) + ". ***\n";	
	broadcast(msg);
}
void welcome(int fd) {
	string msg;
	msg = "****************************************\n** Welcome to the information server. **\n****************************************\n";
	Write(fd, msg.c_str(), msg.size());
}
void logout(int idx) {
	string msg;
	msg = "*** User \'" + string(shmptr->info[idx].name) + "\' left. ***\n";
	close( curfd);
	shmptr->avail[idx] = 0;
	broadcast(msg);
}


void exec_cmd(string input, int idx) {
	parseRes res;

	res = parse(input);
	if (res.np) {
		if (mp.find(rip + res.np) == mp.end()) { 
			int fd[2];
			pipe(fd);
			mp[rip + res.np][0] = fd[0];
			mp[rip + res.np][1] = fd[1];
		}
	}
	if (res.exp) {
		if (mp.find(rip + res.exp) == mp.end()) {
			int fd[2];
			pipe(fd);
			mp[rip + res.exp][0] = fd[0];
			mp[rip + res.exp][1] = fd[1];	
		}
	}
	//if (res.readPipe) {
	
	//} 
	//if (res.writePipe) {
	
	//}
	int cmdlen = res.cmd.size();
	int pipesz = cmdlen - 1;
	
	int lastpid = 0;
	int *prev = nullptr, *cur = nullptr;
	
	for (int i = 0; i < res.cmd.size(); i++) {
		
		char* c = strdup(res.cmd[i].c_str());
		char* tok = strtok(c, " ");
		char* argv[256];
		int cnt = 0;
		while (tok) {
			argv[cnt++] = tok;
			tok = strtok(NULL, " ");
		}

		argv[cnt] = NULL;
		if ( !strncmp(argv[0], "printenv", 8) ) {
			if (cnt < 2) {
				string msg = "Command error\n";
				Write(curfd, msg.c_str(), msg.size());
				continue;
			}
			if ( getenv(argv[1]) != NULL) { 
				string msg = getenv(argv[1]);
				msg += '\n';
				Write (curfd, msg.c_str(), msg.size());
			}
			continue;
		} else if ( !strncmp(argv[0], "setenv", 6) ){
			if (cnt < 3) {
				string msg = "Command error\n";
				Write(curfd, msg.c_str(), msg.size());
				continue;
			}
			if (setenv(argv[1], argv[2], 1) < 0) {
				string msg = "Command error\n";
				Write(curfd, msg.c_str(), msg.size());
			}
			continue;
		} else if ( !strncmp(argv[0], "name", 4) ) {
			name(idx, argv[1]);	
			continue;
		} else if ( !strncmp(argv[0], "yell", 4) ) {
			string msg;
			int found = input.find(argv[1]);
			msg = input.substr(found);
			yell(idx, msg);
			continue;
		} else if ( !strncmp(argv[0], "tell", 4) ) {
			string msg;
			int found = input.find(argv[1]);
			msg = input.substr(found);
		 	tell(idx, atoi(argv[1]), msg);	
			continue;
		} else if ( !strncmp(argv[0], "who", 3) ) {
			who(idx);
			continue;
		}
		if (i != res.cmd.size()-1) {
			cur = new int[2];
			while ( pipe(cur) );
		}
				
		int pid;

		while ( (pid = fork()) == -1) ;
			
		lastpid = pid;
				
		if (mp.find(rip) != mp.end())
			close(mp[rip][1]);
		if (pid == 0) {
			if (res.cmd.size() != 1) { 
				// multiple subprocess
				if (i == 0) { 
					// first process
					if (mp.find(rip) != mp.end()) {
						dup2(mp[rip][0], STDIN_FILENO);
					}
					dup2(cur[1], STDOUT_FILENO);
					close(cur[0]);
					close(cur[1]);
				
				} else if (i == res.cmd.size()-1) { 
					// last process
					dup2(prev[0], STDIN_FILENO);
					close(prev[0]);
					close(prev[1]);
					
					if (res.np) {
						dup2(mp[rip + res.np][1], STDOUT_FILENO);
					} else if (res.exp) {
						dup2(mp[rip + res.exp][1], STDOUT_FILENO);
						dup2(mp[rip + res.exp][1], STDERR_FILENO);
					} else if (res.filename != "") {
						int f;
						if (res.filename != "") {
							f = open(res.filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
						}
						dup2(f, STDOUT_FILENO);
						close(f);
					} else {
						dup2(curfd, STDOUT_FILENO);
						close(curfd);
					}
				} else {
					dup2(prev[0], STDIN_FILENO);
					close(prev[0]);
					close(prev[1]);
					dup2(cur[1], STDOUT_FILENO);	
					close(cur[0]);
					close(cur[1]);
				}
			} else {
				if (mp.find(rip) != mp.end()) {
					dup2(mp[rip][0], STDIN_FILENO);
				}
				if (res.np) {
					dup2(mp[rip + res.np][1], STDOUT_FILENO);
				} else if (res.exp) {
					dup2(mp[rip + res.exp][1], STDOUT_FILENO);
					dup2(mp[rip + res.exp][1], STDERR_FILENO);
				} else if (res.filename != "") {	
					int f;
					if (res.filename != "") {
						f = open(res.filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
					}	
					if ( dup2(f, STDOUT_FILENO)  == -1) {
						perror("dup2 error");
					}
					close(f);
				} else {
					dup2(curfd, STDOUT_FILENO);
					// ??
					close(curfd);
				}
			}
					
			if (execvp(argv[0], argv) < 0 ) {
				// poss
				cerr << "Unknown command: [" << argv[0] << "].\n";
				return ;
			}
		} else {
			if (prev) {
				close(prev[0]);
				close(prev[1]);
				delete[] prev;
			}
			prev = cur;
		}

	}	
	int status;
	if ( res.np == 0 && res.exp == 0) {
		waitpid(lastpid, nullptr, WUNTRACED);
	}
	if (mp.find(rip) != mp.end()) {
		//close(mp[rip][1]);
		close(mp[rip][0]);
		mp.erase(rip);
	}
	return;
}

int main (int argc, char **argv) {
	int ms, ns;
	unsigned int clilen, serv_port;
	char buf[MAXLINE];
	sockaddr_in cliaddr, servaddr;
	cliNum = 0;
	if (argc != 2) {
		cerr << "[usage] ./np_single_proc <port number>\n";
		exit(-1);
	} else {
		serv_port = atoi(argv[1]);
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
	
	if ( (listen(ms, MAXCLI)) < 0 ) {
		perror("Listen error: ");
	}
	//signal(SIGCHLD, sigChild);
	signal(SIGINT, sigHandle);
	clilen = sizeof(cliaddr);
	
	if ( (shmid = shmget(SHMKEY, sizeof(memory), PERMS|IPC_CREAT) ) < 0 ) {
		perror("Shmget error");
	}

	if ( (shmptr = (struct memory *) shmat(shmid, NULL, 0)) == nullptr ) {
		perror("Shmat error");
	}

	

	while (true) {
		int childPid;
		while (cliNum >= 30);
		if ( (ns = accept(ms, (struct sockaddr *) & cliaddr, &clilen)) < 0) {
			perror("Accept error:");
			continue;
		}
		curfd = ns;
		char ip[1024];
		int id = 0;
		inet_ntop(AF_INET, &cliaddr.sin_addr, ip, 1024);
		int port = ntohs(cliaddr.sin_port);
		for (int i = 0; i < 30; i++) {
			if (shmptr->avail[i] == 0) {
				shmptr->avail[i] = 1;
				id = i+1;
				break;
			}
		}
	
		shmptr->userNum += 1;	
		
		
		char buf[MAXLINE];
		memset(buf, 0, sizeof(buf));
		
		while ( (childPid = fork()) < 0 ) ; 
		
		if ( childPid == 0) {
			signal(SIGCHLD, SIG_IGN);
			signal(SIGINT, childINT);
			signal(SIGUSR1, recvUSR1);
			setenv("PATH", "bin:.", 1);
			
			rip = 0;
			mp.clear();

			close(ms);
			while (true) {
				Write(ns, "% ", 2); 
				memset(buf, 0, sizeof(buf));
				string input;
				string tmp;
				tmp = "\n Here\n";
				//Write(ns, tmp.c_str(), tmp.size());
				int cnt = Read(ns, buf, sizeof(buf));
				
				//Write(ns, tmp.c_str(), tmp.size());
				
				for (int i = 0; i < cnt; i++) {
					if (int(buf[i]) == 10 || int(buf[i]) == 13) 
						buf[i] = 0;
				}
				
				input = string(buf);
				
				if (input.empty())
					continue;
				rip++;
				if (input == "exit") {
					// handle logout
					shmptr->userNum -= 1;
					cout << "exit\n";
					return(0);
					//logout();
				} 
				if (input == "test") {
					string msg = to_string(shmptr->userNum);
					Write(ns, msg.c_str(), msg.size());
				}
				//Write(ns, input.c_str(), input.size());
				exec_cmd(input, id-1);
			}

		
		} else {
			string tmp = "(no name)";
			shmptr->info[id-1] = clientInfo(childPid, port, id, tmp.c_str(), ip);
			welcome(ns);
			login(id-1);
			cout << childPid << '\n';
			close(ns);
		}
	
	
	
	
	
	}

}
