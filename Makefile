all: build/inoue

build/inoue: build/inoue.o
	cc -g -o build/inoue -Wall -lcurl build/inoue.o

build/inoue.o: inoue.c
	cc -g -o build/inoue.o -c -Wall inoue.c

run: build/inoue
	build/inoue

.PHONY: all run