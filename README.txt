---------------------------------------------------------------------------------------------------

OS: Linux

---------------------------------------------------------------------------------------------------

To compile and run the server and client respectively:

make server_linux && ./bin/tsamgroup79 4169

make client_linux && ./bin/client 127.0.0.1 4169

---------------------------------------------------------------------------------------------------

To access Skel

ssh <username>@skel.ru.is

To share files between Skel and my PC, I like creating a shared directory to make things easier.
To link the shared directory to a local directory, install sshfs and run the following command:

sshfs matteo22@skel.ru.is:/home/hir.is/matteo22/Project3 /home/meloncrush/SharedDir

To compile and execute the server on Skel:

g++ --std=c++11 server.cpp -o server

---------------------------------------------------------------------------------------------------
Command to find teacher's servers

ps aux|grep i_server

IP: 130.208.243.61
---------------------------------------------------------------------------------------------------
CONNECT,130.208.243.61,4002


---- DONT DELETE UNDERNEATH HERE -----

Instructions:
- To start the server on linux run: make server_linux && ./bin/tsamgroup79 <PORT>
- To start the server on mac run: make server_mac && ./bin/tsamgroup79 <PORT>

- To start the client on linux run: make client_linux && ./bin/client <IP> <PORT>
- To start the client on mac run: make client_mac && ./bin/client <IP> <PORT>

The Client can send the following commands to the server via terminal:
    - CONNECT,<IP>,<PORT> (connects to new server)
    - QUERYSERVERS
    - SEND,<GROUP>,<MSG>
    - FETCH,<GROUP>
    - CLOSE (close client & server)


----- BONUS ------

- We should have received 5 points for the presubmission

- In the file 'numberdecodelog.txt', we have a log of communication with the NUMBER server and received a message
    The decoding of this message is also in the file, so we would get those bonus points

- In the wireshark trace 'Wireshark-trace-from-outside' we show a connection from our server at home (not on skel) 
  to an instructor server, so we should get those bonus points as well