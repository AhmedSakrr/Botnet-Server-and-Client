---------------------------------------------------------------------------------------------------

OS: Linux & Mac

------------- INSTRUCTIONS ---------------

We run the client and server using a Makefile
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
The client also immediately sends 'CLIENT' to the server to indicate it is the client

The server sends keepalives to all connected servers every 90 seconds
If the server receives a keepalive with more than 0 messages waiting, 
it fetches the messages using FETCH_MSGS

The client can use FETCH_MSG to get it's waiting messages
If the client sends a SEND_MSG, the relevant server will be notified with its next KEEPALIVE

All log files have a log for both the client and the server (indicated by SERVER LOG and CLIENT LOG)
The client records all commands it sends to the server and responses it gets
The server records all things it receives from other sources

The client log and server log go to the files 'clientlog.txt' and 'serverlog.txt'


---------------- FILES & ASSIGNMENTS ------------------

NOTE: all assignment log files are in the 'Logs' folder!!

Part 2:
    The file 'server_client_trace.pcap' shows a wireshark trace for all commands from client to server
    This includes all the required commands, but also some extra commands we added such as CONNECT and CLIENT

Part 3:
    The file 'instr_conn_log.txt' shows a successful connection to Instr_2 and Instr_6 (see the JOIN and SERVERS)
    The client also sends a QUERYSERVERS to show the listed servers

Part 4 & 5:
    The file 'send_reply_log_1.txt' shows a connection made to Group 42.
    It shows a message sent from the client to Group 42 and a message successfully received from Group 42.

    The file 'send_reply_log_1.txt' shows a connection made to Group 30.
    It shows a message sent from the client to Group 30 (Hi!! how are you??) and a message received from Group 30
    The message received was an invitation.
    In this log, we also send a message to Group 6, but they do not respond :(


----------------- BONUS --------------------

NOTE: all bonus log files are in the 'Logs' folder!!

- We should have received 5 points for the presubmission

- In the file 'numberdecodelog.txt', we have a log of communication with the NUMBER server and received a message
    The decoding of this message is also in the file, so we would get those bonus points (10 points)

- In the wireshark trace 'Wireshark-trace-from-outside' we show a connection from our server at home (not on skel) 
  to an instructor server, so we should get those bonus points as well (10 points)