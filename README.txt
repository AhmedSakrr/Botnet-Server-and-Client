---------------------------------------------------------------------------------------------------

OS: Linux

---------------------------------------------------------------------------------------------------

To compile and run the server and client respectively:

make server && ./bin/server

make client && ./bin/client

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