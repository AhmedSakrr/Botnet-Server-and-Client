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
#include <fstream>
#include <chrono>
#include <ctime>
#include<iomanip>



// fix SOCK_NONBLOCK for OSX
#ifndef SOCK_NONBLOCK
#include <fcntl.h>
#define SOCK_NONBLOCK O_NONBLOCK
#endif

#define BACKLOG 5 // Allowed length of queue of waiting connections

std::string MY_GROUP = "P3_GROUP_79";

std::ofstream logfile;
std::string logfile_path = "serverlog.txt";

// Simple class for handling connections from clients.
//
// Client(int socket) - socket to send/receive traffic from client.
class Server
{
public:
    int sock;         // socket of client connection
    std::string name; // Limit length of name of client's user
    std::string ip_addr;
    std::string portno = "-1";

    Server(int socket) : sock(socket) {}

    ~Server() {} // Virtual destructor defined for base class
};

// Note: map is not necessarily the most efficient method to use here,
// especially for a server with large numbers of simulataneous connections,
// where performance is also expected to be an issue.
//
// Quite often a simple array can be used as a lookup table,
// (indexed on socket no.) sacrificing memory for speed.

std::map<int, Server *> servers; // Lookup table for per Client information
std::map<std::string, std::vector<std::string>> stored_messages; // Lookup table with waiting messages for each group

char buffer[5000]; // buffer for reading from clients

char SOH = 0x01;
char EOT = 0x04;

struct sockaddr_in own_addr;
std::string own_ip = "0.0.0.0";
std::string own_port;

int listenSock;       // Socket for connections to server
int serverSock;       // Socket of connecting servers
int clientSock;
fd_set openSockets;   // Current open sockets
fd_set readSockets;   // Socket list for select()
fd_set exceptSockets; // Exception socket list
int maxfds;           // Passed to select() as max fd in set

std::string toString(char* a, int size)
{
    int i;
    std::string s = "";
    for (i = 0; i < size; i++) {
        s = s + a[i];
    }
    return s;
}

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
        for (auto const &p : servers)
        {
            *maxfds = std::max(*maxfds, p.second->sock);
        }
    }

    // And remove from the list of open sockets.

    FD_CLR(clientSocket, openSockets);
}

void serverCommand(int serverSocket, char *buffer)
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

    // Incorrect message
    if (tokens[0][0] != SOH) {    
        if (send(serverSocket, "Incorrect Message", strlen("Incorrect Message")-1, 0) < 0)
        {
            perror("Failed to send message to server");
        }
    }

    else if (tokens[0] == "\x01JOIN" && tokens.size() == 2) {
        // Here, we have connected to another server, or it has connected to us
        // We want to send all the servers in our servers dictionary
        // starting with ourself
        servers[serverSocket]->name = tokens[1];
         
        std::string servers_msg = "\x01SERVERS," + MY_GROUP + "," + own_ip + "," + own_port + ";";
        for (auto const &pair : servers) {
            Server *s = pair.second;
            if (s->name != "client") {
                servers_msg.append(s->name + "," + s->ip_addr + "," + s->portno + ";");
            }
        }
        servers_msg.append("\x04");

        if (send(serverSocket, servers_msg.c_str(), strlen(servers_msg.c_str()), 0) < 0)
        {
            perror("Failed to send SERVERS message to server");
        }

        std::cout << "From " << tokens[1] << ": JOIN" << std::endl;
    }

    else if (tokens[0] == "\x01SERVERS") {
        std::string servers_msg = "";
        for (int i = 0; i < tokens.size(); i++) {
            servers_msg.append(tokens[i]);
        }
        std::cout << "From " << tokens[1] << ": " << buffer << std::endl;
    }

    else if (tokens[0] == "\x01STATUSREQ" && tokens.size() == 2) {
        std::cout << "STATUSREQ from " << tokens[1] << std::endl;
        std::string status_resp = "\x01STATUSRESP," + MY_GROUP + tokens[1] + ",";
        for (auto const &m : stored_messages) {
            status_resp.append(m.first);
            status_resp.append(std::to_string(m.second.size()));
        }
        status_resp.append("\x04");
        if (send(serverSocket, status_resp.c_str(), strlen(status_resp.c_str()), 0) < 0)
        {
            perror("Failed to send STATUSRESP message to server");
        }
    }

    else if (tokens[0] == "\x01KEEPALIVE") {
        std::string serv_name = servers[serverSocket]->name;
        std::string keepalive_msg = "FROM " + serv_name + ": " + tokens[0] + " " + tokens[1];
        std::cout << keepalive_msg << std::endl;
    }
}


// Process command from client on the server
void connectToServer(std::string ip_addr, std::string port, std::string group_id)
{
    if (servers.size() > 17) {
        if (send(clientSock, "Server list is full", strlen("Server list is full"), 0) < 0) {
        perror("Failed to send message to client");
        }
    }
    // // Check if already connected, then return
    // for (auto const &pair : servers) {
    //     Server *s = pair.second;
    //     std::cout << s->ip_addr << std::endl;
    //     std::cout << s->portno << std::endl;
    //     if (s->ip_addr.compare(ip_addr) == 0 & s->portno.compare(port) == 0) {
    //         std::cout << "alrdy" << std::endl;
    //         return;
    //     }
    // }

    struct sockaddr_in serv_addr; // Socket address for server
    int connectSock;
    int set = 1;

    struct hostent *server;
    server = gethostbyname(ip_addr.c_str());

    // Need to know the IP address of the server we are connecting to,
	// stores the IP address in binary form in serv_addr.sin_addr
	if( inet_pton(AF_INET, ip_addr.c_str(), &serv_addr.sin_addr) <= 0) {
		perror(" failed to set socket address");
		exit(0);
	}

    // bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    // bcopy((char *)server->h_addr,
    //       (char *)&serv_addr.sin_addr.s_addr,
    //       server->h_length);
    serv_addr.sin_port = htons(atoi(port.c_str()));

    connectSock = socket(AF_INET, SOCK_STREAM, 0);

    // Turn on SO_REUSEADDR to allow socket to be quickly reused after
    // program exit.

    if (setsockopt(connectSock, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0)
    {
        printf("Failed to set SO_REUSEADDR for port %s\n", port.c_str());
        perror("setsockopt failed: ");
    }

    if (connect(connectSock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        // EINPROGRESS means that the connection is still being setup. Typically this
        // only occurs with non-blocking sockets. (The serverSocket above is explicitly
        // not in non-blocking mode, so this check here is just an example of how to
        // handle this properly.)
        if (errno != EINPROGRESS)
        {
            printf("Failed to open socket to server: %s\n", ip_addr.c_str());
            perror("Connect failed: ");
            exit(0);
        }
    }

    // Add new server to the list of open sockets
    FD_SET(connectSock, &openSockets);

    // And update the maximum file descriptor
    maxfds = std::max(maxfds, connectSock);

    // create a new Server to store information.
    servers[connectSock] = new Server(connectSock);
    servers[connectSock]->ip_addr = ip_addr;
    servers[connectSock]->portno = port;

    if (own_ip.compare("0.0.0.0") == 0) {
        socklen_t own_addr_len = sizeof(own_addr);
        int fail = getsockname(connectSock, (struct sockaddr*)&own_addr, &own_addr_len);
        char ip_buf[50];
        const char* p = inet_ntop(AF_INET, &own_addr.sin_addr, ip_buf, 50);
        own_ip = toString(ip_buf, strlen(ip_buf));
    }


    // Send the JOIN message to the new server
    std::string group = "\x01JOIN," + MY_GROUP + "\x04";
    

    if (send(connectSock, group.c_str(), strlen(group.c_str()), 0) < 0)
    {
        perror("Failed to send message to client");
    }
}

void clientCommand(int clientSocket, char *buffer)
{
    buffer[strlen(buffer) - 1] = ','; // We are adding a comma at the end of the buffer because it guarantees that
                                      // that there is always a comma in the buffer. This is needed for the correct
                                      // use of boost::is_any_of() Without the comma it doesn't work on single word
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

    std::string group_prefix = "P3_GROUP_";

    if ((tokens[0].compare("FETCH") == 0) && (tokens.size() == 2))
    {
        std::string group_id = tokens[1];
        std::string resp_fetch_msg;
        bool exists = false;

        // check if name exists
        for (auto const &p: servers) {
            Server *s = p.second;
            if (s->name == group_id && s->name != "client") {
                exists = true;
                break;
            }
        }

        if (!exists) {
            resp_fetch_msg = "SERVER: Group not found " + group_id;
        }
        else
        {
            std::vector<std::string> stored_messages_from_group = stored_messages[group_id];
            if (stored_messages_from_group.size() == 0) 
            {
                resp_fetch_msg = "SERVER: No messages from " + group_id;
            } else {
                resp_fetch_msg = "MESSAGE: " + stored_messages_from_group[0];
                stored_messages_from_group.erase(stored_messages_from_group.begin());
            }
        }
        // std::string not_implemented_msg = "SERVER: Command recognized by server\nNot implemented yet\n";
        if (send(clientSocket, resp_fetch_msg.c_str(), strlen(resp_fetch_msg.c_str()), 0) < 0)
        {
            perror("Failed to send message to client");
        }
    }
    else if (tokens[0].compare("SEND") == 0 && (tokens.size() == 3))
    {
        std::string to_group = tokens[1];
        std::string message_to_send = tokens[2];

        bool matched = false;
        int matching_socket;
        for (auto const &pair : servers) {
            Server *server = pair.second;
            if (server->name.compare(to_group) == 0) {
                matched = true;
                matching_socket = pair.first;
                break;
            }
        }

        if (!matched) {
            std::string no_match = "SERVER: Group not found in active connections\n";
            if (send(clientSocket, no_match.c_str(), strlen(no_match.c_str()), 0) < 0)
            {
                perror("Failed to send message to client");
            }
        }

        else {
            // store message in dictionary for group
            stored_messages[to_group].push_back(message_to_send);

            // send keepalive with number of messages for that group
            std::string keepalive_msg = "\x01KEEPALIVE," + std::to_string(stored_messages[to_group].size());

            if (send(matching_socket, keepalive_msg.c_str(), strlen(keepalive_msg.c_str()), 0) < 0)
            {
                perror("Failed to send message to group");
            }
            std::string sent_msg = "SERVER: Message waiting to be sent to group\n";
            if (send(clientSocket, sent_msg.c_str(), strlen(sent_msg.c_str()), 0) < 0)
            {
                perror("Failed to send message to client");
            }
        }

    }
    else if (tokens[0].compare("QUERYSERVERS") == 0)
    {
        std::string query_message = "SERVERS," + MY_GROUP + "," + own_ip + "," + own_port + ";";
        for (auto const &pair : servers) {
            Server *s = pair.second;
            if (s->name != "client")
                query_message.append(s->name + "," + s->ip_addr + "," + s->portno + ";");
        }
        if (send(clientSocket, query_message.c_str(), strlen(query_message.c_str()), 0) < 0)
        {
            perror("Failed to send query message to client");
        }
    }
    else if (tokens[0].compare("CONNECT") == 0 && (tokens.size() == 3))
    {
        std::string ip = tokens[1];
        std::string port = tokens[2];

        connectToServer(ip, port, MY_GROUP);

        std::string reply_msg = "SERVER: Connected to new server";
        if (send(clientSocket, reply_msg.c_str(), strlen(reply_msg.c_str()), 0) < 0)
        {
            perror("Failed to send message to client");
        }
    }
    else if (tokens[0].compare("CLIENT") == 0) 
    {
        servers[clientSocket]->name = "client";
        servers[clientSocket]->portno = -1;
        clientSock = clientSocket;
    }
    else if (tokens[0].compare("CLOSE") == 0)
    {
        logfile.close();
        exit(0);
    }
    else
    {
        std::string not_implemented_msg = "SERVER: Command not recognized\n";
        if (send(clientSocket, not_implemented_msg.c_str(), strlen(not_implemented_msg.c_str()), 0) < 0)
        {
            perror("Failed to send message to client");
        }
    }
}


void processMessage(int sock, char *buffer) {

    // log the sent message

    // first, get the time
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);

    // stream into string
    std::ostringstream oss;
    oss << std::put_time(&tm, "%d-%m-%Y %H:%M:%S");
    auto time_str = oss.str();


    // server command
    if (buffer[0] == SOH) {
        std::string log_msg = time_str + ": " + toString(buffer, strlen(buffer));
        logfile << log_msg + "\n";
        serverCommand(sock, buffer);
    }
    // client command
    else {
        std::string log_msg = "From Client: " + time_str + ": " + toString(buffer, strlen(buffer));
        logfile << log_msg + "\n";
        std::cout << "client command: " << buffer << std::endl;
        clientCommand(sock, buffer);
    }
}


int main(int argc, char *argv[])
{
    bool finished;
    struct sockaddr_in client;
    socklen_t clientLen;

    if (argc != 2)
    {
        printf("Usage: server <ip port>\n");
        exit(0);
    }

    // Setup socket for server to listen to

    listenSock = open_socket(atoi(argv[1]));
    printf("Listening on port: %d\n", atoi(argv[1]));

    own_port = toString(argv[1], 4);

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

    logfile.open (logfile_path);
    logfile << "START SERVER LOG\n";

    bool found_client = false;

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
            // First, accept any new connections to the server on the listening socket
            // we only want 16 connections maximum, but we also have the client in the list
            if (FD_ISSET(listenSock, &readSockets) && servers.size() < 18)
            {
                serverSock = accept(listenSock, (struct sockaddr *)&client,
                                    &clientLen);
                printf("accept** %i\n", serverSock);

                // Add new client to the list of open sockets
                FD_SET(serverSock, &openSockets);

                // And update the maximum file descriptor
                maxfds = std::max(maxfds, serverSock);

                // create a new client to store information.
                servers[serverSock] = new Server(serverSock);
                servers[serverSock]->portno = "-1";

                char ip_str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(client.sin_addr), ip_str, INET_ADDRSTRLEN);
                servers[serverSock]->ip_addr = toString(ip_str, (int) (sizeof(ip_str) / sizeof(char)));

                if (own_ip.compare("0.0.0.0") == 0) {
                    socklen_t own_addr_len = sizeof(own_addr);
                    int fail = getsockname(serverSock, (struct sockaddr*)&own_addr, &own_addr_len);
                    char ip_buf[50];
                    const char* p = inet_ntop(AF_INET, &own_addr.sin_addr, ip_buf, 50);
                    own_ip = toString(ip_buf, strlen(ip_buf));
                }

                // send initial JOIN
                std::string joined_msg = "\x01JOIN," + MY_GROUP + "\x04";
                if (send(serverSock, joined_msg.c_str(), strlen(joined_msg.c_str()), 0) < 0)
                {
                    perror("Failed to send JOIN message to new server");
                }

                // Decrement the number of sockets waiting to be dealt with
                n--;
            }

            // Now check for commands from servers
            std::list<Server *> disconnectedServers;
            for (auto const &pair : servers)
            {
                Server *server = pair.second;


                if (FD_ISSET(server->sock, &readSockets))
                {
                    // recv() == 0 means client has closed connection

                    memset(buffer, 0, sizeof(buffer));
                    if (recv(server->sock, buffer, sizeof(buffer), MSG_DONTWAIT) == 0)
                    {
                        disconnectedServers.push_back(server);
                        closeClient(server->sock, &openSockets, &maxfds);
                    }
                    // We don't check for -1 (nothing received) because select()
                    // only triggers if there is something on the socket for us.
                    else
                    {
                        if (strlen(buffer) != 0) {
                            processMessage(server->sock, buffer);
                        }
                        memset(buffer, 0, sizeof(buffer));
                    }
                }
            }
            // Remove client from the clients list
            for (auto const &c : disconnectedServers)
                servers.erase(c->sock);
        }
    }
}
