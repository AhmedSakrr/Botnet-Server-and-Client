CC=g++
CFLAGS=--std=c++11
SOURCE_DIR=./src
BIN_DIR=./bin

client_linux: $(SOURCE_DIR)/client.cpp
	$(CC) $(CFLAGS) -I /usr/local/boost_1_80_0 $(SOURCE_DIR)/client.cpp -o $(BIN_DIR)/client 

server_linux: $(SOURCE_DIR)/server.cpp
	$(CC) $(CFLAGS) -I /usr/local/boost_1_80_0 $(SOURCE_DIR)/server.cpp -o $(BIN_DIR)/tsamgroup79 

client_mac: $(SOURCE_DIR)/client.cpp
	$(CC) $(CFLAGS) -I /opt/homebrew/Cellar/boost/1.80.0/include $(SOURCE_DIR)/client.cpp -o $(BIN_DIR)/client 

server_mac: $(SOURCE_DIR)/server.cpp
	$(CC) $(CFLAGS) -I /opt/homebrew/Cellar/boost/1.80.0/include $(SOURCE_DIR)/server.cpp -o $(BIN_DIR)/server