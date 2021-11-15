#include "pack.hpp"

int cliNum;
int rip;
map<int, int[2]> mp;
int shmid;

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

void sigChild(int signo) {
	pid_t pid;
	int stat;
	while((pid = waitpid(-1, &stat, WNOHANG) > 0 ));
	return;
}

void exec_cmd(string input, int ns) {
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
	
	for (int i = 0; i < cmd.size(); i++) {
		
		char* c = strdup(cmd[i].c_str());
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
				Write(ns, msg.c_str(), msg.size());
				continue;
			}
			if ( getenv(argv[1]) != NULL) { 
				string msg = getenv(argv[1]);
				msg += '\n';
				Write (ns, msg.c_str(), msg.size());
			}
			continue;
		} else if ( !strncmp(argv[0], "setenv", 6) ){
			if (cnt < 3) {
				string msg = "Command error\n";
				Write(ns, msg.c_str(), msg.size());
				continue;
			}
			if (setenv(argv[1], argv[2], 1) < 0) {
				string msg = "Command error\n";
				Write(ns, msg.c_str(), msg.size());
				continue;
			}
			continue;
		} 
			
		if (i != cmd.size()-1) {
			cur = new int[2];
			while ( pipe(cur) );
		}
				
		int pid;

		while ( (pid = fork()) == -1) ;
			
		lastpid = pid;
				
		if (mp.find(rip) != mp.end())
			close(mp[rip][1]);
		if (pid == 0) {
			if (cmd.size() != 1) { 
				// multiple subprocess
				if (i == 0) { 
					// first process
					if (mp.find(rip) != mp.end()) {
						dup2(mp[rip][0], STDIN_FILENO);
					}
					dup2(cur[1], STDOUT_FILENO);
					close(cur[0]);
					close(cur[1]);
				
				} else if (i == cmd.size()-1) { 
					// last process
					dup2(prev[0], STDIN_FILENO);
					close(prev[0]);
					close(prev[1]);
					
					if (np) {
						dup2(mp[rip+np][1], STDOUT_FILENO);
					} else if (exp) {
						dup2(mp[rip+exp][1], STDOUT_FILENO);
						dup2(mp[rip+exp][1], STDERR_FILENO);
					} else if (filename != "") {
						int f;
						if (filename != "") {
							f = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
						}
						dup2(f, STDOUT_FILENO);
						close(f);
					} else {
						dup2(newsockfd, STDOUT_FILENO);
						//?
						close(newsockfd);
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
				if (np) {
					dup2(mp[rip+np][1], STDOUT_FILENO);
				} else if (exp) {
					dup2(mp[rip+exp][1], STDOUT_FILENO);
					dup2(mp[rip+exp][1], STDERR_FILENO);
				} else if (filename != "") {	
					int f;
					if (filename != "") {
						f = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
					}	
					if ( dup2(f, STDOUT_FILENO)  == -1) {
						perror("dup2 error");
					}
					close(f);
				} else {
					dup2(ns, STDOUT_FILENO);
					// ??
					close(ns);
				}
			}
					
			if (execvp(argv[0], argv) < 0 ) {
				// poss
				cerr << "Unknown command: [" << argv[0] << "].\n";
				return -1;
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
			setenv("PATH", "bin:.", 1);
			
			rip = 0;
			mp.clear();

			close(ms);
			while (true) {
				Write(ns, "% ", 2); 
				memset(buf, 0, sizeof(buf));
				string input;
				int cnt = Read(ns, buf, sizeof(buf));
				
				
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
				Write(ns, input.c_str(), input.size());
				//exec_cmd(input);
			}

		
		} else {
			string tmp = "(no name)";
			shmptr->info[id-1] = clientInfo(childPid, port, id, tmp.c_str(), ip);
			cout << childPid << '\n';
			close(ns);
		}
	
	
	
	
	
	}

}
