#Makefile
# Created on: Aug 4, 2025
#     Author: swalton
#
# * Copyright (c) 2025, IAS Publishing, LLC

BIN_DIR=./bin
CXXFLAGS=-std=c++14 -Wall -g3 -Wno-multichar

#SRC_H=$(wildcard ./*.h)
#SRC_C=$(wildcard ./*.c)
OBJ_C=$(patsubst %.c, $(BIN_DIR)/%.o, $(SRC_C))
SRC_CPP=$(wildcard ./*.cpp)

OBJ_CPP=$(patsubst %.cpp, $(BIN_DIR)/%.o, $(SRC_CPP))

$(BIN_DIR)/actiontarget_ping: $(BIN_DIR) $(OBJ_CPP) $(OBJ_C)
	g++ -o $@ $(OBJ_CPP) $(OBJ_C)

$(BIN_DIR):
	mkdir -p $@

$(OBJ_CPP): $(BIN_DIR)/%.o: %.cpp
	g++ -c $(CXXFLAGS) $< -o $@

$(OBJ_C): $(BIN_DIR)/%.o: %.c $(SRC_H)
	gcc -c $(CFLAGS) $< -o $@

clean:
	@rm -rf $(BIN_DIR)

all: $(BIN_DIR)/actiontarget_ping

install:
	sudo cp $(BIN_DIR)/actiontarget_ping /usr/local/bin/actiontarget_ping
	sudo cp action_target.service /etc/systemd/system
	sudo echo "systemd.user:systemd:9999:999::/home/systemd.user:/usr/sbin/nologin" | newusers
	sudo mkdir -p /home/systemd.user/action-target
	sudo chown -R systemd.user:999 /home/systemd.user

#$(BIN_DIR)/xmlgen
#
#$(BIN_DIR)/xmlgen: xmlgen.c
#	gcc -c $(CFLAGS) $< -o $@
