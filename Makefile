CC := clang
CXX := clang++

CFLAGS   := $(CFLAGS) -fwrapv -fno-strict-aliasing
CXXFLAGS := $(CFLAGS) -std=c++20 -fno-exceptions -fno-rtti

WFLAGS := -Wall -Wextra -Werror=return-type

SRC := $(wildcard *.cpp *.hpp)

.PHONY: clean run test

run: olc.exe
	./olc.exe

tlsf.o: lib/tlsf.c lib/tlsf.h
	$(CC) $(CFLAGS) -O2 -c lib/tlsf.c -o tlsf.o

olc.exe: $(SRC) tlsf.o
	$(CXX) $(CXXFLAGS) $(WFLAGS) -O0 -o olc.exe main.cpp tlsf.o

TEST_SRC := $(wildcard tests/*.cpp tests/*.hpp)

tests.exe: $(SRC) $(TEST_SRC) tlsf.o
	$(CXX) $(CXXFLAGS) $(WFLAGS) -g -Og -o tests.exe tests/main.cpp tlsf.o

test: tests.exe
	./tests.exe

CLEAN_GLOB := *.o *.exe *.pdb *.exp *.ilk

clean:
	rm -rf  $(CLEAN_GLOB) || del  $(CLEAN_GLOB)
