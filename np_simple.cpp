#include "pack.hpp"


vector<string> parse (string cmd) {
	vector<string> res;
	string delim = "|!>";
	char *c = strdup(cmd.c_str());
	char *tok = strtok(c, delim.c_str());

	while (tok) {
		res.pb(string(tok));
		tok = strtok(NULL, delim.c_str());
	}
	for (int i = 0; i < res.size(); i++) {
		res[i].erase(0, res[i].find_first_not_of(" "));
		res[i].erase(res[i].find_last_not_of(" ") + 1);
	}
	return res;
}

void sigHandler(int signo) {
	pid_t pid;
	int stat;
	while((pid = waitpid(-1, &stat, WNOHANG) > 0 ));
	return;
}

int main (int argc, char ** argv) {
	
	int sockfd, newsockfd, childpid;
	unsigned int clilen;
	unsigned int serv_port;
	struct sockaddr_in cli_addr, serv_addr;
	
if (argc == 2) 
		serv_port = atoi(argv[1]);
	else {
		cout << "[usage] ./np_single_proc <port number>\n";
		exit(-1);
	}	

	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Socket error:");
	}
	
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family 	  = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port 		  = htons(serv_port);
	int reuse = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*) &reuse, sizeof(int));
	if ( bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0 ) {
		perror("Bind error:");
	}
	
	if (listen(sockfd, MAX_CLI) ) {
		perror("listen error:");
	}
	/// slave start from here
	while (true) {
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd < 0)
			perror("Accept error:");
		while ( ( childpid = fork()) < 0);
		
		// After fork	
		if (childpid == 0) {
			// Slave doing their job
			signal(SIGCHLD, sigHandler);
			setenv("PATH", "bin:.", 1);
			int rip = 0;
			map<int, int[2]> mp;
			close(sockfd);	
			dup2(newsockfd, STDOUT_FILENO);
			dup2(newsockfd, STDERR_FILENO);
			while (1) {
				string prompt = "% ";
				write(STDOUT_FILENO, "% ", 2);
				
				char buf[16000];
				memset(buf, 0, sizeof(buf));
				string input;
				int cnt = Read(newsockfd, buf, sizeof(buf));
				for (int i = 0; i < cnt; i++) {
					if ( int(buf[i]) == 10 || int(buf[i]) == 13)
						buf[i] = 0;
				}
				
				if (!cnt) {
					break;
				}	
				input = string(buf);
				if (input.empty())
					continue;
				rip++;
				vector<string> cmd = parse(input);
				int np = 0, exp = 0;
				int idx1 = input.find_last_of("|");
				int idx2 = input.find_last_of("!");
				int idx3 = input.find_last_of(">");
				string snp, sexp, filename;
				
				if (idx1 != -1) {
					snp = input.substr(idx1+1);	
				}
				if (idx2 != -1) {
					sexp = input.substr(idx2+1);	
				}
				if (idx3 != -1) {
					filename = cmd.back();
					cmd.pop_back();
				}

				if ( !snp.empty() && strncmp(snp.c_str(),  " ", 1) ) {
					np = stoi(snp);
				} else if ( !sexp.empty() && strncmp(sexp.c_str(), " ", 1) ) {
					exp = stoi(sexp);
				}

				if (np) { 
					cmd.pop_back();
					if (mp.find(rip + np) == mp.end()) { 
						int fd[2];
						pipe(fd);
						mp[rip + np][0] = fd[0];
						mp[rip + np][1] = fd[1];
					}
				}
				if (exp) {
					cmd.pop_back();
					if (mp.find(rip + exp) == mp.end()) {
						int fd[2];
						pipe(fd);
						mp[rip + exp][0] = fd[0];
						mp[rip + exp][1] = fd[1];	
					}
				}

				int cmdlen = cmd.size();
				int pipeSz = cmdlen - 1;

				if (cmd[0] == "exit") {
					return 0;
				} 
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
							Write(newsockfd, msg.c_str(), msg.size());
							continue;
						}
						if ( getenv(argv[1]) != NULL) { 
							string msg = getenv(argv[1]);
							msg += '\n';
							Write (newsockfd, msg.c_str(), msg.size());
						}
						continue;
					} else if ( !strncmp(argv[0], "setenv", 6) ){
						if (cnt < 3) {
							string msg = "Command error\n";
							Write(newsockfd, msg.c_str(), msg.size());
							continue;
						}
						if (setenv(argv[1], argv[2], 1) < 0) {
							string msg = "Command error\n";
							Write(newsockfd, msg.c_str(), msg.size());
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
								dup2(newsockfd, STDOUT_FILENO);
								// ??
								close(newsockfd);
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
				if ( np == 0 && exp == 0) {
					waitpid(lastpid, nullptr, WUNTRACED);
				}
				if (mp.find(rip) != mp.end()) {
					close(mp[rip][1]);
					close(mp[rip][0]);
					mp.erase(rip);
				}
			}
			close(newsockfd);
		} else {
			close(newsockfd);
			waitpid(childpid, nullptr, WUNTRACED);	
		}		

	}


}
