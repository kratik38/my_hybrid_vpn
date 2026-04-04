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
	LDFLAGS += -L/usr/local/lib -L/usr/lib
endif

#files
SOURCES = src/main.cpp src/TunInterface.cpp src/UdpSocket.cpp src/HybridKEM.cpp src/AesGcm.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = build/vpn_app

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p build
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf build/*.o src/*.o $(TARGET)

test_sandbox: dir
	$(CXX) src/test_crypto.cpp src/HybridKEM.cpp build/test_crypto $(CXXFLAGS) $(LDFLAGS)
	./build/test_crypto
