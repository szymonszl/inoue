all: build/inoue

build/inoue: build/inoue.o
	cc -g -o build/inoue -Wall -lcurl build/inoue.o

build/inoue.o: inoue.c
	mkdir -p build
	cc -g -o build/inoue.o -c -Wall inoue.c

run: build/inoue
	mkdir -p build/data
	build/inoue build/data

.PHONY: all run