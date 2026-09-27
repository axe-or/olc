CXX    ?= clang
CFLAGS := $(CFLAGS) -std=c++20 -fno-exceptions -fno-rtti -fwrapv -fno-strict-aliasing -O0

SRC := $(wildcard *.cpp *.hpp)

.PHONY: clean run test

run: olc.exe
	./olc.exe

olc.exe: $(SRC)
	$(CXX) $(CFLAGS) -o olc.exe main.cpp

test: arena_test.exe
	./arena_test.exe

arena_test.exe: tests/arena_test.cpp base.hpp base.cpp
	$(CXX) $(CFLAGS) -o $@ tests/arena_test.cpp

clean:
	rm -rf *.o *.exe *.pdb *.exp *.ilk
