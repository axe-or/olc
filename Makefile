CC     := clang
CFLAGS := $(CFLAGS) -fwrapv -fno-strict-aliasing -O0
CXXFLAGS := $(CFLAGS) -std=c++20 -fno-exceptions -fno-rtti 

WFLAGS := -Wall -Wextra -Werror=return-type

SRC := $(wildcard *.cpp *.hpp)

.PHONY: clean run test

run: olc.exe
	./olc.exe

lib/rpmalloc.o: lib/rpmalloc.c
	$(CC) $(CFLAGS) $(WFLAGS) -o lib/rpmalloc.o -c lib/rpmalloc.c

olc.exe: $(SRC) lib/rpmalloc.o
	$(CC) $(CXXFLAGS) $(WFLAGS) -o olc.exe lib/rpmalloc.o main.cpp

clean:
	rm -rf *.o *.exe *.pdb *.exp *.ilk
