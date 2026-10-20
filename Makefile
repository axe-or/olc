CC := clang
CXX := clang++

CFLAGS   := $(CFLAGS) -fwrapv -fno-strict-aliasing
CXXFLAGS := $(CFLAGS) -std=c++20 -fno-exceptions -fno-rtti

WFLAGS := -Wall -Wextra -Werror=return-type -Wno-microsoft-exception-spec

WASI_SDK ?= lib/wasi-sdk

SRC := $(wildcard *.cpp *.hpp)

.PHONY: clean run test wasm

run: olc.exe
	@./olc.exe

tlsf.o: lib/tlsf.c lib/tlsf.h
	$(CC) $(CFLAGS) -O2 -c lib/tlsf.c -o tlsf.o

olc.exe: $(SRC) tlsf.o
	$(CXX) $(CXXFLAGS) $(WFLAGS) -O0 -o olc.exe main.cpp tlsf.o

WASM_FLAGS := --target=wasm32-wasip1 --sysroot=$(WASI_SDK)/share/wasi-sysroot

tlsf.wasm.o: lib/tlsf.c lib/tlsf.h
	$(WASI_SDK)/bin/clang $(WASM_FLAGS) $(CFLAGS) -O2 -c lib/tlsf.c -o tlsf.wasm.o

olc.wasm: $(SRC) tlsf.wasm.o
	$(WASI_SDK)/bin/clang++ $(WASM_FLAGS) $(CXXFLAGS) $(WFLAGS) -O2 -o olc.wasm main.cpp tlsf.wasm.o

wasm: olc.wasm tests.wasm

TEST_SRC :=$(wildcard tests/*.cpp tests/*.hpp)

tests.wasm: $(TEST_SRC) tlsf.wasm.o
	$(WASI_SDK)/bin/clang++ $(WASM_FLAGS) $(CXXFLAGS) $(WFLAGS) -O2 -o tests.wasm tests/main.cpp tlsf.wasm.o

tests.exe: $(SRC) $(TEST_SRC) tlsf.o
	$(CXX) $(CXXFLAGS) $(WFLAGS) -g -Og -o tests.exe tests/main.cpp tlsf.o

test: tests.exe
	./tests.exe

CLEAN_GLOB := *.o *.exe *.wasm *.pdb *.exp *.ilk

clean:
	rm -rf  $(CLEAN_GLOB) || del  $(CLEAN_GLOB)
