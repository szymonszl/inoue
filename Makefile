all: build/inoue

build/inoue: build/inoue.o build/buffer.o
	cc -g -o build/inoue -Wall -lcurl build/inoue.o build/buffer.o

build/inoue.o: src/inoue.c
	mkdir -p build
	cc -g -o build/inoue.o -c -Wall src/inoue.c

build/buffer.o: src/buffer.c
	mkdir -p build
	cc -g -o build/buffer.o -c -Wall src/buffer.c

# debug targets; they assume build/data exists and has a proper config file
run: build/inoue
	build/inoue build/data
gdb: build/inoue
	gdb --args build/inoue build/data

.PHONY: all run gdb