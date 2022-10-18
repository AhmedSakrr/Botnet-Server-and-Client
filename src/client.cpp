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
#include <vector>
#include <thread>

#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <map>
#include <chrono>
#include <ctime>

#include<iomanip>

using namespace std;


ofstream logfile;
std::string logfile_path = "log.txt";


std::string toString(char* a, int size)
{
    int i;
    std::string s = "";
    for (i = 0; i < size; i++) {
        s = s + a[i];
    }
    return s;
}

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
    }
}

bool sendClientCommand(int serverSocket, char *buffer)
{ 
    buffer[strlen(buffer) - 1] = ','; // We are adding a comma at the end of the buffer because it guarantees that
                                      // that there is always a comma in the buffer. This is needed for the correct
                                      // use of boost::is_any_of() Without the comma it doesn't work on single words
                                      // commands, like QUERYSERVERS

    std::vector<std::string> tokens;
    std::string s = toString(buffer, strlen(buffer));
    std::string delimiter = ",";
    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        token = s.substr(0, pos);
        tokens.push_back(token);
        s.erase(0, pos + delimiter.length());
    }

    bool command_is_correct = false;

    // Checks if the last token is empty, removes it if true
    if(tokens.back().size() == 0){
        tokens.pop_back();
    }

    // Groups should use the syntax P3_GROUP_n where "n" is your group number
    string group_prefix = "P3_GROUP_";

    if ((tokens[0].compare("FETCH") == 0) && (tokens.size() == 2)) 
    {
        // printf("CLIENT: Command FETCH recognized\n");
        command_is_correct = true;
    }
    else if (tokens[0].compare("SEND") == 0 && (tokens.size() == 3)) 
    {
        // printf("CLIENT: Command SEND recognized\n");
        command_is_correct = true;

        // log the sent message

        // first, get the time
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);

        // stream into string
        std::ostringstream oss;
        oss << std::put_time(&tm, "%d-%m-%Y %H:%M:%S");
        auto time_str = oss.str();

        std::string log_msg = "SENT TO " + tokens[1] + ": " + tokens[2] + "; (" + time_str + ")";
        std::cout << log_msg << std::endl;
        logfile << log_msg + "\n";

    }
    else if (tokens[0].compare("QUERYSERVERS") == 0)
    {
        // printf("CLIENT: Command QUERYSERVERS recognized\n");
        command_is_correct = true;
    }
    else if (tokens[0].compare("CONNECT") == 0 && (tokens.size() == 3)) 
    {
        // printf("CLIENT: Command CONNECT recognized\n");
        command_is_correct = true;
    }
    else if (tokens[0].compare("CLOSE") == 0) {
        logfile.close();
        exit(0);
    }
    else
    {
        printf("CLIENT: Unknown command.\n");
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
    int set = 1;                  // Toggle for setsockopt

    if (argc != 3 and argc != 4)
    {
        printf("Usage: client <ip  port>\n");
        printf("Or usage: client <ip port logfile>");
        printf("Ctrl-C to terminate\n");
        exit(0);
    }

    if (argc == 4) 
        logfile_path = toString(argv[3], strlen(argv[3]));


    hints.ai_family = AF_INET;    // IPv4 only addresses
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
        std::string client_key = "CLIENT";
        if (send(serverSocket, client_key.c_str(), strlen(client_key.c_str()), 0) < 0)
        {
            perror("Failed to send CLIENT key to server");
        }
    }

    logfile.open (logfile_path);
    logfile << "START LOG\n";

    // Listen and print replies from server
    thread serverThread(listenServer, serverSocket);

    finished = false;
    while (!finished)
    {
        bzero(buffer, sizeof(buffer));

        fgets(buffer, sizeof(buffer), stdin);

        finished = sendClientCommand(serverSocket, buffer);
    }

    logfile.close();
}
