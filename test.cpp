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

using namespace std;
#define SERV_PORT 7001
#define MAX_CLI 10
void sig1(int signo) {
	int pid, stat;
	cout << "sig1 \n";
	while((pid = waitpid(-1, &stat, WNOHANG) > 0 ));
		return;

}

void sig2(int signo) {
	int pid, stat;
	cout << "sig4 \n";
	while((pid = waitpid(-1, &stat, WNOHANG) > 0 ));
		return;


}

int main () {
	int sockfd, newsockfd, childpid;
	unsigned int clilen;
	struct sockaddr_in cli_addr, serv_addr;
	
	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Socket error:");
	}
	
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family 	  = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port 		  = htons(SERV_PORT);

	if ( bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0 ) {
		perror("Bind error:");
	}
	
	listen(sockfd, MAX_CLI);
	for (;;) {
		clilen = sizeof(cli_addr);
		if ( (newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen))  < 0) {
			perror("accept error");
		}
		if ( (childpid = fork()) < 0) {
			perror("fork error");
		} else if (childpid == 0) {
			close(sockfd);
			string msg = "Hello\n";
			write(newsockfd, msg.c_str(), msg.size());
			exit(0);
		}
		close(newsockfd);
	}

}
