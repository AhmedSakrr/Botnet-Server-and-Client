CC=g++
CFLAGS=--std=c++11
SOURCE_DIR=./src
BIN_DIR=./bin

client: $(SOURCE_DIR)/client.cpp
	$(CC) $(CFLAGS) $(SOURCE_DIR)/client.cpp -o $(BIN_DIR)/client 

server: $(SOURCE_DIR)/server.cpp
	$(CC) $(CFLAGS) $(SOURCE_DIR)/server.cpp -o $(BIN_DIR)/server 