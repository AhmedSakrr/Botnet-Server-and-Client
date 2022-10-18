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


// fix SOCK_NONBLOCK for OSX
#ifndef SOCK_NONBLOCK
#include <fcntl.h>
#define SOCK_NONBLOCK O_NONBLOCK
#endif

#define BACKLOG 5 // Allowed length of queue of waiting connections

std::string MY_GROUP = "P3_GROUP_69";

// Simple class for handling connections from clients.
//
// Client(int socket) - socket to send/receive traffic from client.
class Server
{
public:
    int sock;         // socket of client connection
    std::string name; // Limit length of name of client's user
    std::string ip_addr;
    std::string portno;

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
std::string own_ip;
std::string own_port;

int listenSock;       // Socket for connections to server
int serverSock;       // Socket of connecting servers
int clientSock;       // Socket of connecting client
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
            if (s->name != "client")
                servers_msg.append(s->name + "," + s->ip_addr + "," + s->portno + ";");
        }
        servers_msg.append("\x04");

        if (send(serverSocket, servers_msg.c_str(), strlen(servers_msg.c_str()), 0) < 0)
        {
            perror("Failed to send SERVERS message to server");
        }
    }

    else if (tokens[0] == "\x01SERVERS") {

    }

    else if (tokens[0] == "\x01STATUSREQ" && tokens.size() == 2) {
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
        std::cout << tokens[0] << tokens[1] << " from " << serv_name << std::endl;
    }
}


// Process command from client on the server
void connectToServer(std::string ip_addr, std::string port, std::string group_id)
{
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
    servers[connectSock] = new Server(serverSock);
    servers[connectSock]->ip_addr = ip_addr;
    servers[connectSock]->portno = port;

    socklen_t own_addr_len = sizeof(own_addr);
    int fail = getsockname(connectSock, (struct sockaddr*)&own_addr, &own_addr_len);
    char ip_buf[50];
    const char* p = inet_ntop(AF_INET, &own_addr.sin_addr, ip_buf, 50);
    own_ip = toString(ip_buf, strlen(ip_buf));


    // Send the JOIN message to the new server
    std::string group = "\x01JOIN," + MY_GROUP + "\x04";
    

    if (send(connectSock, group.c_str(), strlen(group.c_str()), 0) < 0)
    {
        perror("Failed to send message to client");
    }

    memset(&buffer, 0, sizeof(buffer));

    // Receive the JOIN message from the server
    int nread = read(connectSock, buffer, sizeof(buffer));

    if (nread == 0) // Server has dropped us
    {
        printf("Over and Out\n");
        exit(0);
    }
    else if (nread > 0)
    {
        printf("Server Response: %s\n", buffer);
        // process the JOIN command and return SERVERS
        serverCommand(connectSock, buffer);
    }


    // Receive the SERVERS command
    memset(&buffer, 0, sizeof(buffer));
    nread = read(connectSock, buffer, sizeof(buffer));

    if (nread == 0) // Server has dropped us
    {
        printf("Over and Out\n");
        exit(0);
    }
    else if (nread > 0)
    {
        printf("Server Response: %s\n", buffer);
        serverCommand(connectSock, buffer);
    }
}


void clientCommand(int clientSocket, char *buffer)
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

    std::string group_prefix = "P3_GROUP_";

    if ((tokens[0].compare("FETCH") == 0) && (tokens.size() == 2))
    {
        std::string group_id = tokens[1];
        std::string not_implemented_msg = "SERVER: Command recognized by server\nNot implemented yet\n";
        if (send(clientSocket, not_implemented_msg.c_str(), strlen(not_implemented_msg.c_str()), 0) < 0)
        {
            perror("Failed to send message to client");
        }
        // TODO: Implement FETCH command
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

        std::string reply_msg = "SERVER: Command executed by server";
        if (send(clientSocket, reply_msg.c_str(), strlen(reply_msg.c_str()), 0) < 0)
        {
            perror("Failed to send message to client");
        }
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
    // server command
    if (buffer[0] == SOH) {
        std::cout << "server command" << std::endl;
        serverCommand(sock, buffer);
    }
    // client command
    else {
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

    bool found_client = false;

    // while (!found_client) {

    //     // Get modifiable copy of readSockets
    //     readSockets = exceptSockets = openSockets;

    //     // Look at sockets and see which ones have something to be read()
    //     int l = select(maxfds + 1, &readSockets, NULL, &exceptSockets, NULL);

    //     if (FD_ISSET(listenSock, &readSockets))
    //         {
    //             clientSock = accept(listenSock, (struct sockaddr *)&client,
    //                                 &clientLen);
    //             printf("accept***\n");
    //             // Add new client to the list of open sockets
    //             FD_SET(clientSock, &openSockets);

    //             // And update the maximum file descriptor
    //             maxfds = std::max(maxfds, clientSock);

    //             found_client = true;

    //             printf("Client connected on server: %d\n", clientSock);
    //         }
    // }

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
            if (FD_ISSET(listenSock, &readSockets))
            {
                std::cout << "someone wants connect" << std::endl;
                serverSock = accept(listenSock, (struct sockaddr *)&client,
                                    &clientLen);
                printf("accepting new server\n");

                // Add new client to the list of open sockets
                FD_SET(serverSock, &openSockets);

                // And update the maximum file descriptor
                maxfds = std::max(maxfds, serverSock);

                // create a new client to store information.
                servers[serverSock] = new Server(serverSock);

                servers[serverSock]->name = "client";

                // Decrement the number of sockets waiting to be dealt with
                n--;
            }

            // First, check for command from client
            if (FD_ISSET(clientSock, &readSockets)) {
                memset(buffer, 0, sizeof(buffer));
                if (recv(clientSock, buffer, sizeof(buffer), MSG_DONTWAIT) == 0)
                {
                    std::cout << "Client disconnected" << std::endl;
                    exit(0);
                }
                // We don't check for -1 (nothing received) because select()
                // only triggers if there is something on the socket for us.
                else
                {
                    if (strlen(buffer) != 0) {
                        processMessage(clientSock, buffer);
                    }
                    memset(buffer, 0, sizeof(buffer));
                }
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
