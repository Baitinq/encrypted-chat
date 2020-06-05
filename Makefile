CXX = g++

CXXFLAGS = -Wall

main:
	$(CXX) $(CXXFLAGS) server.cpp -o server
	$(CXX) $(CXXFLAGS) client.cpp -o client
