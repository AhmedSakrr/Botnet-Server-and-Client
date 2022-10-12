//
// Simple chat client for TSAM-409
//
// Command line: ./chat_client 4000
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
#include <thread>

#include <iostream>
#include <sstream>
#include <thread>
#include <map>

#include <bits/stdc++.h>

// Threaded function for handling responses from server

void listenServer(int serverSocket)
{
    int nread;         // Bytes read from socket
    char buffer[5000]; // Buffer for reading input

    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        nread = read(serverSocket, buffer, sizeof(buffer));

        if (nread == 0) // Server has dropped us
        {
            printf("Over and Out\n");
            exit(0);
        }
        else if (nread > 0)
        {
            printf("%s\n", buffer);
        }
        printf("here\n");
    }
}

bool sendClientCommand(int serverSocket, char buffer[])
{
    bool command_is_correct = false;
    std::vector<std::string> tokens;

    // Returns a pointer to the first token in the string
    char *token = strtok(buffer, ",");

    // Cycle through the tokens, dividing them based on the "," and adding them to the tokens vector
    while (token != NULL)
    {
        // printf("%s\n", token);
        tokens.push_back(token);

        token = strtok(NULL, ",");
    }

    // Groups should use the syntax P3_GROUP_n where "n" is your group number
    std::string group_prefix = "P3_GROUP_";

    if ((tokens[0].compare("FETCH") == 0) && tokens[1].rfind(group_prefix, 0) == 0 && (tokens.size() == 2)) // syntax: FETCH GROUPID
    {
        printf("CLIENT: Command FETCH recognized\n");
        command_is_correct = true;
    }
    else if (tokens[0].compare("SEND") == 0 && tokens[1].rfind(group_prefix, 0) == 0 && (tokens.size() == 3)) // syntax SEND GROUPID msg
    {
        printf("CLIENT: Command SEND recognized\n");
        command_is_correct = true;
    }
    else if (tokens[0].compare("QUERYSERVERS") == 0) // syntax QUERYSERVERS
    {
        // TODO: For now, it only works if you write "QUERYSERVERS,"
        printf("CLIENT: Command QUERYSERVERS recognized\n");
        command_is_correct = true;
    }
    else
    {
        printf("CLIENT: Unknown command. Will not be send to the Server\n");
    }

    if (command_is_correct)
    {
        if (send(serverSocket, buffer, strlen(buffer), 0) == -1)
        {
            perror("CLIENT: send() to server failed: ");
            return true;
        }
        return false;
    }

    return false;
}

int main(int argc, char *argv[])
{
    struct addrinfo hints, *svr;  // Network host entry for server
    struct sockaddr_in serv_addr; // Socket address for server
    int serverSocket;             // Socket used for server
    int nwrite;                   // No. bytes written to server
    char buffer[5000];            // buffer for writing to server
    bool finished;
    int set = 1; // Toggle for setsockopt

    if (argc != 3)
    {
        printf("Usage: client <ip  port>\n");
        printf("Ctrl-C to terminate\n");
        exit(0);
    }

    hints.ai_family = AF_INET; // IPv4 only addresses
    hints.ai_socktype = SOCK_STREAM;

    memset(&hints, 0, sizeof(hints));

    if (getaddrinfo(argv[1], argv[2], &hints, &svr) != 0)
    {
        perror("getaddrinfo failed: ");
        exit(0);
    }

    struct hostent *server;
    server = gethostbyname(argv[1]);

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(atoi(argv[2]));

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    // Turn on SO_REUSEADDR to allow socket to be quickly reused after
    // program exit.

    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0)
    {
        printf("Failed to set SO_REUSEADDR for port %s\n", argv[2]);
        perror("setsockopt failed: ");
    }

    if (connect(serverSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        // EINPROGRESS means that the connection is still being setup. Typically this
        // only occurs with non-blocking sockets. (The serverSocket above is explicitly
        // not in non-blocking mode, so this check here is just an example of how to
        // handle this properly.)
        if (errno != EINPROGRESS)
        {
            printf("Failed to open socket to server: %s\n", argv[1]);
            perror("Connect failed: ");
            exit(0);
        }
    }

    // Listen and print replies from server
    std::thread serverThread(listenServer, serverSocket);

    finished = false;
    while (!finished)
    {
        bzero(buffer, sizeof(buffer));

        printf("Enter a command (or press ctrl+C to exit): ");
        fgets(buffer, sizeof(buffer), stdin);
        printf("\n");

        finished = sendClientCommand(serverSocket, buffer);
    }
}
