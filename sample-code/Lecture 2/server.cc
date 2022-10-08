#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <algorithm>
#include <signal.h>

int		listenSock;
int		clientSock;
fd_set		openSockets;
fd_set		readSockets;
fd_set		exceptSockets;
int		maxfds;

int		portno = 55556;
char		buffer    [10240];

void send_to_all(char *buffer, int len)
{
	for (int i = 0; i <= maxfds; i++) {
		if (FD_ISSET(i, &openSockets) && i != listenSock) {
			write(i, buffer, len);
		}
	}
}

	int
main()
{
	//signal(SIGPIPE, SIG_IGN);

	//socket() is the same on both sides(client / server)
	if ((listenSock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Failed to create socket");
		return (-1);
	}
	//Turn on SO_REUSEADDR to allow socket to be quickly reused after program exit.

	int		set = 1;
	if (setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0) {
		perror("Failed to set SO_REUSEADDR:");
	}
	struct sockaddr_in sk_addr;
	//Initialise memory
	memset(&sk_addr, 0, sizeof(sk_addr));

	//Set type of connection
	sk_addr.sin_family = AF_INET;
	sk_addr.sin_addr.s_addr = INADDR_ANY;
	sk_addr.sin_port = htons(portno);

	//And bind address / port to socket
	if (bind(listenSock, (struct sockaddr *)&sk_addr, sizeof(sk_addr)) < 0) {
		perror(" Failed  to  bind  to socket:  ");
		return (-1);
	}
	listen(listenSock, 5);

	FD_ZERO(&openSockets);
	FD_SET(listenSock, &openSockets);
	maxfds = listenSock;
	//there is only one socket so far

	while (1) {
		readSockets = exceptSockets = openSockets;

		int fd;
		//Get a list of sockets waiting to be serviced(blocks while non are waiting)
		int n = select(maxfds + 1, &readSockets, NULL, &exceptSockets, NULL);
		if (n < 0) {
			perror("select failed");
			exit(1);
		}
		printf("select: data on %d socket descriptors\n", n);

		for (fd = 0; fd <= maxfds; fd++) {
			if (FD_ISSET(fd, &readSockets)) {
				if (fd == listenSock) {
					printf("accepting new connection\n");
					struct sockaddr_in client;
					unsigned int clientLen = sizeof(client);
					clientSock = accept(listenSock, (struct sockaddr *)&client, &clientLen);

					//Add new client socket to the list of sockets being monitored
					FD_SET(clientSock, &openSockets);

					//update the max fd in our socket set
					maxfds = std::max(maxfds, clientSock);
				} else {
					//data from a client
					printf("data from a client on fd %d\n", fd);
					int nbytes = recv(fd, buffer, sizeof(buffer), 0);
					if (nbytes <= 0) {
						//no data = >end of connection / connection error
						printf("clearing fd %d due to irecv returning %d\n", fd, nbytes);
						close(fd);
						FD_CLR(fd, &openSockets);
					} else {
						sleep(1);
						send_to_all(buffer, nbytes);
					}
				}
			}
			if (FD_ISSET(fd, &exceptSockets)) {
				printf("fd %d is in exceptinal state\n", fd);
			}
		}

	}
}
