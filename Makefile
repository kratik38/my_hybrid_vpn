#compiler
CXX = g++

#detection 
UNAME_S := $(shell uname -s)

#standard flags
CXXFLAGS = -std=c++17 -Wall -Wextra -I./include
#global linker to the liboqs library
LDFLAGS = -loqs -lssl -lcrypto

#os specific flags
ifeq ($(UNAME_S),Darwin)
	#mac
	CXXFLAGS += -D__APPLE__
	BREW_PREFIX := $(shell brew --prefix)
	CXXFLAGS += -I$(BREW_PREFIX)/include -I$(BREW_PREFIX)/opt/openssl@3/include
LDFLAGS += -L$(BREW_PREFIX)/lib -L$(BREW_PREFIX)/opt/openssl@3/lib
else
	#linux
	CXXFLAGS += -D__linux__
endif

#files
SRC = src/main.cpp src/TunInterface.cpp src/UdpSocket.cpp src/HybridKEM.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = build/vpn_app

#rules
all: dir $(TARGET)

dir:
	mkdir -p build
	
$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $(TARGET) $(LDFLAGS)
	
clean:
	rm -f src/*.o build/vpn_app

test_sandbox: dir
	$(CXX) src/test_crypto.cpp src/HybridKEM.cpp build/test_crypto $(CXXFLAGS) $(LDFLAGS)
	./build/test_crypto
