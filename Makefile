CXX = g++
CXXFLAGS = -std=c++17 -Wall

all: host client

host: host.cpp
	$(CXX) $(CXXFLAGS) -o host host.cpp

client: client.cpp
	$(CXX) $(CXXFLAGS) -o client client.cpp

clean:
	rm -f host client
