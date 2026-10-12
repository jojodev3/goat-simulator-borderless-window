# Build a 32-bit dinput8.dll with MinGW-w64 (i686-w64-mingw32-g++).
CC  = i686-w64-mingw32-gcc
CXX = i686-w64-mingw32-g++
MH  = lib/minhook
OBJ = build/buffer.o build/hook.o build/trampoline.o build/hde32.o

all: build/dinput8.dll

build:
	mkdir -p build

build/%.o: $(MH)/src/%.c | build
	$(CC) -O2 -c -I$(MH)/include $< -o $@

build/hde32.o: $(MH)/src/hde/hde32.c | build
	$(CC) -O2 -c -I$(MH)/include $< -o $@

build/dinput8.dll: src/goat_borderless.cpp src/dinput8.def $(OBJ)
	$(CXX) -O2 -std=c++17 -shared -static -static-libgcc -static-libstdc++ \
	  -I$(MH)/include src/goat_borderless.cpp $(OBJ) src/dinput8.def \
	  -o $@ -Wl,--kill-at

clean:
	rm -rf build

.PHONY: all clean
