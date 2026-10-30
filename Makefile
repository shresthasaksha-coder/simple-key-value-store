CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic

all: kvstore

kvstore: main.o kvstore.o
	$(CXX) $(CXXFLAGS) -o kvstore main.o kvstore.o

main.o: main.cpp kvstore.h
	$(CXX) $(CXXFLAGS) -c main.cpp

kvstore.o: kvstore.cpp kvstore.h
	$(CXX) $(CXXFLAGS) -c kvstore.cpp

clean:
	rm -f *.o kvstore
