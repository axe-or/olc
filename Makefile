CXX    := clang
CFLAGS := $(CFLAGS) -std=c++20 -fno-exceptions -fno-rtti -fwrapv -fno-strict-aliasing -O0
WFLAGS := -Wall -Wextra -Werror=return-type

SRC := $(wildcard *.cpp *.hpp)

.PHONY: clean run test

run: olc.exe
	./olc.exe

olc.exe: $(SRC)
	$(CXX) $(CFLAGS) $(WFLAGS) -o olc.exe main.cpp

clean:
	rm -rf *.o *.exe *.pdb *.exp *.ilk
