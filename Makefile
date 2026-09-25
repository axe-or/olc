CXX    ?= clang
CFLAGS := $(CFLAGS) -std=c++20 -fno-exceptions -fno-rtti -fwrapv -fno-strict-aliasing -O0

SRC := $(wildcard *.cpp *.hpp)

.PHONY: clean run

run: olc.exe
	./olc.exe

olc.exe: $(SRC)
	$(CXX) $(CFLAGS) -o olc.exe main.cpp

clean:
	rm -rf *.o *.exe *.pdb *.exp *.ilk

