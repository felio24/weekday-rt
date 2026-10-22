CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -pedantic

.PHONY: all clean

all: main

main: main.cpp *.h
	$(CXX) $(CXXFLAGS) main.cpp -o main

clean:
	rm -f main
