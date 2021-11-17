#include "pack.hpp"

int cliNum;
int rip;
map<int, int[2]> mp;
int shmid;
int curfd;
int up[35];

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
	//cout << "RECVUSR1\n";
	string msg = string(shmptr->msg);
	//cout << shmptr->msg << endl;
	Write(curfd, msg.c_str(), msg.size());
	
}

void recvUSR2(int signo) {
	//recv fifo req
	int from = shmptr->pipeNo[0], to = shmptr->pipeNo[1];
	string fname = "./user_pipe/fifo" + to_string(from) + "to" + to_string(to);
	up[from] = open(fname.c_str(), O_RDONLY);
	
}

void sigChild(int signo) {
	pid_t pid;
	int stat;
	while((pid = waitpid(-1, &stat, WNOHANG) > 0 ));
	return;
}
void broadcast(string msg) {
	sem_wait(&SEM);
	//memset(shmptr->msg, 0, sizeof(shmptr->msg));
	strcpy(shmptr->msg, msg.c_str());

	for (int i = 0; i < 30; i++) {
		if (shmptr->avail[i] == 1) {
			//sem_post(&SEM);
			kill(shmptr->info[i].pid, SIGUSR1);
		}
	}
	sem_post(&SEM);
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
	sem_wait(&SEM);
	strcpy(shmptr->info[idx].name, s.c_str());
	sem_post(&SEM);
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
		if (shmptr->avail[i] == 1 && shmptr->info[i].id == to) {
			msg = "*** " + string(shmptr->info[idx].name) + " told you ***: " + s + "\n";
			sem_wait(&SEM);
			//memset(shmptr->msg, 0, sizeof(shmptr->msg));
			strcpy(shmptr->msg, msg.c_str());
			sem_post(&SEM);
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
void welcome() {
	string msg;
	msg = "****************************************\n** Welcome to the information server. **\n****************************************\n";
	Write(curfd, msg.c_str(), msg.size());
}
void logout(int idx) {
	string msg;
	msg = "*** User \'" + string(shmptr->info[idx].name) + "\' left. ***\n";
	close(curfd);
	for (int i = 1; i <= 30; i++) {
		if ( shmptr->userPipe[i][idx+1] ) {
			string fname = "./user_pipe/fifo" + to_string(i) + "to" + to_string(idx+1);
			unlink(fname.c_str());
			sem_wait(&SEM);
			shmptr->userPipe[i][idx+1] = 0;
			sem_post(&SEM);
		}
	}
	
	for (int i = 1; i <= 30; i++) {
		if (shmptr->userPipe[idx+1][i] ) {
			string fname = "./user_pipe/fifo" + to_string(idx+1) + "to" + to_string(i);
			
			unlink(fname.c_str());
			sem_wait(&SEM);
			shmptr->userPipe[idx+1][i] = 0;
			sem_post(&SEM);
		}
	}

	sem_wait(&SEM);
	shmptr->avail[idx] = 0;
	sem_post(&SEM);
	broadcast(msg);
	//kill all pipe
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
	string readname;
	int rfd = -1;
	
	if (res.readPipe) {
		//error !FIFO存在 or USER不在 or PIPE IN
		readname = "./user_pipe/fifo" + to_string(res.readPipe) + "to" + to_string(idx+1);
		struct stat *sb;
		if (shmptr->avail[res.readPipe-1] == 0) {
			//	USER不存在
			string msg;
			msg = "*** Error: user #" + to_string(res.readPipe) + " does not exist yet. ***\n";
			Write(curfd, msg.c_str(), msg.size());
		} else if ( shmptr->userPipe[res.readPipe][idx+1] == 0) {
			//	FIFO不存在
			string msg;
			msg = "*** Error: the pipe #" + to_string(res.readPipe) + "->#" + to_string(idx+1) + " does not exist yet. ***\n";
			Write(curfd, msg.c_str(), msg.size());
		} else {
			// succues
			//string msg;
			//msg = "*** " + string(shmptr->info[idx].name) + " (#" + to_string(shmptr->info[idx].id) + ") just received from ";
			//msg += string(shmptr->info[res.readPipe-1].name) + " (#" +to_string(res.readPipe) + ") by \'" + input + "\' ***\n";
			//broadcast(msg);
			rfd = up[res.readPipe];
		}
	} 
	
	int wfd = -1;

	if (res.writePipe) {
		//FIFO存在 and USER不在 and PIPE 成功
		string fname = "./user_pipe/fifo" + to_string(idx+1) + "to" + to_string(res.writePipe);
		struct stat *sb;
		if (shmptr->avail[res.writePipe-1] == 0) {
			// user dont exist
			string msg;
			msg = "*** Error: user #" + to_string(res.writePipe) + " does not exist yet. ***\n";
			Write(curfd, msg.c_str(), msg.size());
		} else if ( shmptr->userPipe[idx+1][res.writePipe] == 0) {
			sem_wait(&SEM);
			shmptr->userPipe[idx+1][res.writePipe] = 1;
			shmptr->pipeNo[0] = idx+1, shmptr->pipeNo[1] = res.writePipe;
			sem_post(&SEM);

			if ( ( mknod(fname.c_str(), S_IFIFO | PERMS, 0)) < 0 ) {
				//perror("OPEN FIFO");
			}
			
			kill(shmptr->info[res.writePipe-1].pid, SIGUSR2);
			//cout << "open f\n";
			wfd = open(fname.c_str(), O_WRONLY);

			//string msg;
			//msg = "*** " + string(shmptr->info[idx].name) + " (#" + to_string(shmptr->info[idx].id) + ") just piped \'" + input + "\' to "; 
			//msg += string(shmptr->info[res.writePipe-1].name) + " (#" + to_string(shmptr->info[res.writePipe-1].id) + ") ***\n";
			//broadcast(msg);
		} else {
			//FIFO exist
			string msg;
			msg = "*** Error: the pipe #" + to_string(idx+1) + "->#" + to_string(res.writePipe) + " already exists. ***\n";
			Write(curfd, msg.c_str(), msg.size());
		}
	}
	string bmsg;
	if (rfd != -1 ) {
		bmsg = "*** " + string(shmptr->info[idx].name) + " (#" + to_string(shmptr->info[idx].id) + ") just received from ";
		bmsg += string(shmptr->info[res.readPipe-1].name) + " (#" +to_string(res.readPipe) + ") by \'" + input + "\' ***\n";

	}
	if (wfd != -1) {
		bmsg += "*** " + string(shmptr->info[idx].name) + " (#" + to_string(shmptr->info[idx].id) + ") just piped \'" + input + "\' to "; 
		bmsg += string(shmptr->info[res.writePipe-1].name) + " (#" + to_string(shmptr->info[res.writePipe-1].id) + ") ***\n";
	}
	if (rfd != -1 || wfd != -1) {
		broadcast(bmsg);	
	}
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
			int found = input.find(argv[2]);
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
				
		if (mp.find(rip) != mp.end()) {
			close(mp[rip][1]);
		}
		if (pid == 0) {
			if (res.cmd.size() != 1) { 
				// multiple subprocess
				if (i == 0) { 
					// first process
					if (mp.find(rip) != mp.end()) {
						dup2(mp[rip][0], STDIN_FILENO);
					} else if (res.readPipe) {
						if (rfd == -1) {
							int d = open("/dev/null", O_RDWR);
							dup2(d, STDIN_FILENO);
						} else {
							dup2(up[res.readPipe], STDIN_FILENO);
						}
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
					} else if (res.writePipe) {
						if (wfd == -1) {
							int d = open("/dev/null", O_RDWR);
							dup2(d, STDOUT_FILENO);
						} else {
							dup2(wfd, STDOUT_FILENO);
						}
					} else {
						dup2(curfd, STDOUT_FILENO);
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
				if (res.readPipe) {
					if (rfd == -1) {
						int d = open("/dev/null", O_RDWR);
						dup2(d, STDIN_FILENO);
					} else {
						dup2(up[res.readPipe], STDIN_FILENO);
					}
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
				} else if (res.writePipe) { 
					if (wfd == -1) {
						int d = open("/dev/null", O_RDWR);
						dup2(d, STDOUT_FILENO);
					} else {
						dup2(wfd, STDOUT_FILENO);
					}
				} else {
					dup2(curfd, STDOUT_FILENO);
				}
			}
					
			if (execvp(argv[0], argv) < 0 ) {
				string msg = "Unknown command: [" + string(argv[0]) + "].\n";
				Write(curfd, msg.c_str(), msg.size());
				
				close(curfd);
				exit(-1);
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
	if ( res.np == 0 && res.exp == 0 && res.writePipe == 0) {
		waitpid(lastpid, nullptr, WUNTRACED);
	}
	if (res.writePipe) {
		close(wfd);
	}
	if (mp.find(rip) != mp.end()) {
		//close(mp[rip][1]);
		close(mp[rip][0]);
		mp.erase(rip);
	}
	if (res.readPipe) {
		sem_wait(&SEM);
		shmptr->userPipe[res.readPipe][idx+1] = 0;
		sem_post(&SEM);
		close(rfd);
		unlink(readname.c_str());
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
	if ( (sem_init(&(shmptr->sem), 1, 1)) == -1) {
		perror("Sem_init");
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
				id = i+1;
				break;
			}
		}
		
		sem_wait(&SEM);	
		shmptr->userNum += 1;	
		sem_post(&SEM);

		signal(SIGUSR1, recvUSR1);
		signal(SIGUSR2, recvUSR2);	
		char buf[MAXLINE];
		memset(buf, 0, sizeof(buf));
		
		while ( (childPid = fork()) < 0 ) ; 
		
		if ( childPid == 0) {
			signal(SIGCHLD, SIG_IGN);
			signal(SIGINT, childINT);

			setenv("PATH", "bin:.", 1);
			dup2(curfd, STDOUT_FILENO);
			dup2(curfd, STDERR_FILENO);	
			//init
			rip = 0;
			mp.clear();
			memset(up, 0, sizeof(up));
			welcome();	
			string msg;
			msg = "*** User \'" + string("(no name)")+ "\' entered from " + string(ip) + ":" + to_string(port) + ". ***\n";	
			Write(curfd, msg.c_str(), msg.size());
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
					sem_wait(&SEM);
					shmptr->userNum -= 1;
					sem_post(&SEM);
					cout << "exit\n";
					logout(id-1);
					return(0);
					//logout();
				} 
				//Write(ns, input.c_str(), input.size());
				//cerr << "Input: " << input << '\n';
				exec_cmd(input, id-1);
				//cerr << "finish\n";
			}

		
		} else {
			
			string tmp = "(no name)";
			sem_wait(&SEM);
			shmptr->info[id-1] = clientInfo(childPid, port, id, tmp.c_str(), ip);
			sem_post(&SEM);

			login(id-1);
			cout << childPid << '\n';
			sem_wait(&SEM);
			shmptr->avail[id-1] = 1;
			sem_post(&SEM);
			close(ns);
		}
	
	
	
	
	
	}

}
