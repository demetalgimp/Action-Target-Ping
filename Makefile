#Makefile
# Created on: Aug 4, 2025
#     Author: swalton

BIN_DIR=./bin
CXXFLAGS=-std=c++14 -Wall -g3 -Wno-multichar

#SRC_H=$(wildcard ./*.h)
#SRC_C=$(wildcard ./*.c)
OBJ_C=$(patsubst %.c, $(BIN_DIR)/%.o, $(SRC_C))
SRC_CPP=$(wildcard ./*.cpp)

OBJ_CPP=$(patsubst %.cpp, $(BIN_DIR)/%.o, $(SRC_CPP))

$(BIN_DIR)/main: $(BIN_DIR) $(OBJ_CPP) $(OBJ_C)
	g++ -o $@ $(OBJ_CPP) $(OBJ_C)

$(BIN_DIR):
	mkdir -p $@

$(OBJ_CPP): $(BIN_DIR)/%.o: %.cpp
	g++ -c $(CXXFLAGS) $< -o $@
	
$(OBJ_C): $(BIN_DIR)/%.o: %.c $(SRC_H)
	gcc -c $(CFLAGS) $< -o $@
	
clean:
	@rm -rf $(BIN_DIR)
	
all: $(BIN_DIR)/main 

#$(BIN_DIR)/xmlgen
#
#$(BIN_DIR)/xmlgen: xmlgen.c
#	gcc -c $(CFLAGS) $< -o $@
