#compiler
CXX = g++

#detection 
UNAME_S := $(shell uname -s)

#standard flags
CXXFLAGS = -std=c++17 -Wall -Wextra -I./include

#os specific flags
ifeq ($(UNAME_S),Darwin)
	#mac
	CXXFLAGS += -D__APPLE__
else
	#linux
	CXXFLAGS += -D__linux__
endif

#files
SRC = src/main.cpp src/TunInterface.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = build/vpn_app

#rules
all: dir $(TARGET)

dir:
	 mkdir -p build
	
$(TARGET): $(OBJ)
	 $(CXX) $(OBJ) -o $(TARGET)
	
clean:
	 rm -f src/*.o build/vpn_app
