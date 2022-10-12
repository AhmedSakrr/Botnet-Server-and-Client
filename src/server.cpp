//
// Simple chat server for TSAM-409
//
// Command line: ./chat_server 4000
//
// Author: Jacky Mallett (jacky@ru.is)
//
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <algorithm>
#include <map>
#include <vector>
#include <list>
#include <cstring>

#include <iostream>
#include <sstream>
#include <thread>
#include <map>

#include <unistd.h>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <algorithm>
#include <map>
#include <vector>
#include <thread>

#include <iostream>
#include <sstream>
#include <thread>
#include <map>

#include <bits/stdc++.h>

// fix SOCK_NONBLOCK for OSX
#ifndef SOCK_NONBLOCK
#include <fcntl.h>
#define SOCK_NONBLOCK O_NONBLOCK
#endif

#define BACKLOG 5 // Allowed length of queue of waiting connections

// Simple class for handling connections from clients.
//
// Client(int socket) - socket to send/receive traffic from client.
class Client
{
public:
    int sock;         // socket of client connection
    std::string name; // Limit length of name of client's user

    Client(int socket) : sock(socket) {}

    ~Client() {} // Virtual destructor defined for base class
};

// Note: map is not necessarily the most efficient method to use here,
// especially for a server with large numbers of simulataneous connections,
// where performance is also expected to be an issue.
//
// Quite often a simple array can be used as a lookup table,
// (indexed on socket no.) sacrificing memory for speed.

std::map<int, Client *> clients; // Lookup table for per Client information

// Open socket for specified port.
//
// Returns -1 if unable to create the socket for any reason.

int open_socket(int portno)
{
    struct sockaddr_in sk_addr; // address settings for bind()
    int sock;                   // socket opened for this port
    int set = 1;                // for setsockopt

// Create socket for connection. Set to be non-blocking, so recv will
// return immediately if there isn't anything waiting to be read.
#ifdef __APPLE__
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Failed to open socket");
        return (-1);
    }
#else
    if ((sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0)
    {
        perror("Failed to open socket");
        return (-1);
    }
#endif

    // Turn on SO_REUSEADDR to allow socket to be quickly reused after
    // program exit.

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0)
    {
        perror("Failed to set SO_REUSEADDR:");
    }
    set = 1;
#ifdef __APPLE__
    if (setsockopt(sock, SOL_SOCKET, SOCK_NONBLOCK, &set, sizeof(set)) < 0)
    {
        perror("Failed to set SOCK_NOBBLOCK");
    }
#endif
    memset(&sk_addr, 0, sizeof(sk_addr));

    sk_addr.sin_family = AF_INET;
    sk_addr.sin_addr.s_addr = INADDR_ANY;
    sk_addr.sin_port = htons(portno);

    // Bind to socket to listen for connections from clients

    if (bind(sock, (struct sockaddr *)&sk_addr, sizeof(sk_addr)) < 0)
    {
        perror("Failed to bind to socket:");
        return (-1);
    }
    else
    {
        return (sock);
    }
}

// Close a client's connection, remove it from the client list, and
// tidy up select sockets afterwards.

void closeClient(int clientSocket, fd_set *openSockets, int *maxfds)
{

    printf("Client closed connection: %d\n", clientSocket);

    // If this client's socket is maxfds then the next lowest
    // one has to be determined. Socket fd's can be reused by the Kernel,
    // so there aren't any nice ways to do this.

    close(clientSocket);

    if (*maxfds == clientSocket)
    {
        for (auto const &p : clients)
        {
            *maxfds = std::max(*maxfds, p.second->sock);
        }
    }

    // And remove from the list of open sockets.

    FD_CLR(clientSocket, openSockets);
}

// Process command from client on the server

void clientCommand(int clientSocket, fd_set *openSockets, int *maxfds, char *buffer)
{
    printf("Buffer contains %s\n", buffer);

    printf("Inside clientCommand\n");

    std::vector<std::string> tokens;
    char *token = strtok(buffer, ",");

    std::cout << "The buffer contains: " << buffer << std::endl;

    printf("Before while tokenizer\n");
    while (token != NULL)
    {
        printf("Insie the while: %s\n", token);
        tokens.push_back(token);

        token = strtok(NULL, ",");
    }

    std::string group_prefix = "P3_GROUP_";

    printf("Before the first if\n");
    if ((tokens[0].compare("FETCH") == 0) && tokens[1].rfind(group_prefix, 0) == 0 && (tokens.size() == 2)) // syntax: FETCH GROUPID
    {
        printf("Inside the FETCH if\n");
        std::string group = tokens[1];
        std::string not_implemented_msg = "SERVER: Command recognized by server\nNot implemented yet\n";
        if (send(clientSocket, not_implemented_msg.c_str(), strlen(not_implemented_msg.c_str()), 0) < 0)
        {
            perror("Failed to send message to client");
        }
        // TODO: Implement FETCH command
    }
    else if (tokens[0].compare("SEND") == 0 && tokens[1].rfind(group_prefix, 0) == 0 && (tokens.size() == 3)) // syntax SEND GROUPID msg
    {
        std::string group = tokens[1];
        std::string message = tokens[2];
        std::string not_implemented_msg = "SERVER: Command recognized by server\nNot implemented yet\n";
        if (send(clientSocket, not_implemented_msg.c_str(), strlen(not_implemented_msg.c_str()), 0) < 0)
        {
            perror("Failed to send message to client");
        }
        // TODO: Implement SEND command
    }
    else if (tokens[0].compare("QUERYSERVERS") == 0)
    {
        std::string not_implemented_msg = "SERVER: Command recognized by server\nNot implemented yet\n";
        if (send(clientSocket, not_implemented_msg.c_str(), strlen(not_implemented_msg.c_str()), 0) < 0)
        {
            perror("Failed to send message to client");
        }
        // std::cout << "Who is logged on" << std::endl;
        // std::string msg;

        // for (auto const &names : clients)
        // {
        //     msg += names.second->name + ",";
        // }
        // // Reducing the msg length by 1 loses the excess "," - which
        // // granted is totally cheating.
        // send(clientSocket, msg.c_str(), msg.length() - 1, 0);
    }
    else
    {
        printf("Inside the else for command not recognized.\n");
        std::string not_implemented_msg = "SERVER: Command not recognized\n";
        if (send(clientSocket, not_implemented_msg.c_str(), strlen(not_implemented_msg.c_str()), 0) < 0)
        {
            perror("Failed to send message to client");
        }
        std::cout << "Unknown command from client:" << buffer << std::endl;
    }

    printf("End of the clientCommand\n");
}

int main(int argc, char *argv[])
{
    bool finished;
    int listenSock;       // Socket for connections to server
    int clientSock;       // Socket of connecting client
    fd_set openSockets;   // Current open sockets
    fd_set readSockets;   // Socket list for select()
    fd_set exceptSockets; // Exception socket list
    int maxfds;           // Passed to select() as max fd in set
    struct sockaddr_in client;
    socklen_t clientLen;
    char buffer[5000]; // buffer for reading from clients

    if (argc != 2)
    {
        printf("Usage: chat_server <ip port>\n");
        exit(0);
    }

    // Setup socket for server to listen to

    listenSock = open_socket(atoi(argv[1]));
    printf("Listening on port: %d\n", atoi(argv[1]));

    if (listen(listenSock, BACKLOG) < 0)
    {
        printf("Listen failed on port %s\n", argv[1]);
        exit(0);
    }
    else
    // Add listen socket to socket set we are monitoring
    {
        FD_ZERO(&openSockets);
        FD_SET(listenSock, &openSockets);
        maxfds = listenSock;
    }

    finished = false;

    while (!finished)
    {
        // Get modifiable copy of readSockets
        readSockets = exceptSockets = openSockets;
        memset(buffer, 0, sizeof(buffer));

        // Look at sockets and see which ones have something to be read()
        int n = select(maxfds + 1, &readSockets, NULL, &exceptSockets, NULL);

        if (n < 0)
        {
            perror("select failed - closing down\n");
            finished = true;
        }
        else
        {
            // First, accept  any new connections to the server on the listening socket
            if (FD_ISSET(listenSock, &readSockets))
            {
                clientSock = accept(listenSock, (struct sockaddr *)&client,
                                    &clientLen);
                printf("accept***\n");
                // Add new client to the list of open sockets
                FD_SET(clientSock, &openSockets);

                // And update the maximum file descriptor
                maxfds = std::max(maxfds, clientSock);

                // create a new client to store information.
                clients[clientSock] = new Client(clientSock);

                // Decrement the number of sockets waiting to be dealt with
                n--;

                printf("Client connected on server: %d\n", clientSock);
            }
            // Now check for commands from clients
            std::list<Client *> disconnectedClients;
            while (n-- > 0)
            {
                for (auto const &pair : clients)
                {
                    Client *client = pair.second;

                    if (FD_ISSET(client->sock, &readSockets))
                    {
                        // recv() == 0 means client has closed connection
                        if (recv(client->sock, buffer, sizeof(buffer), MSG_DONTWAIT) == 0)
                        {
                            disconnectedClients.push_back(client);
                            closeClient(client->sock, &openSockets, &maxfds);
                        }
                        // We don't check for -1 (nothing received) because select()
                        // only triggers if there is something on the socket for us.
                        else
                        {
                            std::cout << buffer << std::endl;
                            clientCommand(client->sock, &openSockets, &maxfds, buffer);
                        }
                    }
                }
                // Remove client from the clients list
                for (auto const &c : disconnectedClients)
                    clients.erase(c->sock);
            }
        }
    }
}
