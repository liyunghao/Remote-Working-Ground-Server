#include "pack.hpp"

int cliNum;

void sigHandle(int signo) {
	//clear shm
}
void sigChild(int signo) {
	cliNum --;
	pid_t pid;
	int stat;
	while((pid = waitpid(-1, &stat, WNOHANG) > 0 ));
	return;
	
}
int main (int argc, char **argv) {
	int ms, ns;
	unsigned int clilen, serv_port;
	char buf[MAXLINE];
	sockaddr_in cliaddr, servaddr;
	cliNum = 0;
	if (argv != 2) {
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
	signal(SIGCHLD, sigChild);
	signal(SIGINT, sigHandle);
	clilen = sizeof(cli_addr);
	
	while (true) {
		int childPid;
		while (cliNum >= 30);
		if ( (ns = accept(ms, (struct sockaddr *) & cli_addr, &clilen)) < 0) {
			perror("Accept error:");
			continue;
		} 
		
		while ( (childPid = fork()) < 0 ) ; 
		
		if ( childPid == 0) {
			while()		
	
		
		} else {
			cout << childPid << '\n';
			close(ns);
		}
	
	
	
	
	
	}

}
