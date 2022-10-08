#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

char buffer[4096];

int main(int argc, char **argv) {

	// For later use
	if(argc<3) { printf("usage: server <serverip> <serverport>\n"); exit(1); }
	int portno = atoi( argv[2] );
	char *server = argv[1];


	int sock;
	// socket() is the same on both sides (client/server)
	if( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Failed to create socket"); return (-1);
	}

	// Setup socket address structure for connection struct
	struct sockaddr_in serv_addr;

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET ;
	serv_addr.sin_port = htons( portno );

	// Need to know the IP address of the server

	if( inet_pton(AF_INET, server, &serv_addr.sin_addr) <= 0)
	{
		perror(" failed to set socket address"); exit(0);
	}

	// Connect to remote address
	if( connect( sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		perror(" Could  not connect");
	}


	/*
	   struct sockaddr_in server_addr; struct hostent *server;

	   server = gethostbyname("skel.ru.is"); // map name to host entity

	// Fill in fields for server_addr
	memset (&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons( portno );

	memcpy((char *)&server_addr.sin_addr.s_addr, 
	(char *)server->h_addr,
	server->h_length);

	// Connect to remote address
	connect( sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
	 */

	// Don’t send the NULL character at end of string
	send( sock, "Hello World", sizeof("Hello World") - 1, 0);


	int nread = read( sock, buffer, sizeof(buffer) );
	printf("%s\n", buffer);

	// … [usually above code is repeated in some loop]
	close( sock );
}

