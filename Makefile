VERSION = $(shell git describe --always --tags)

OBJS = build/inoue.o build/buffer.o build/util.o build/cfg.o \
	build/leaderboard.o build/game.o build/log.o build/http_curl.o
DEPS = src/inoue.h src/json.h src/winunistd.h
CFLAGS = -Wall -g `curl-config --cflags` -DINOUE_VER=\"$(VERSION)\"
LDFLAGS = `curl-config --libs`

all: build/inoue

build/inoue: $(OBJS)
	@mkdir -p build
	$(CC) -o $@ $^ $(LDFLAGS)

$(OBJS): build/%.o: src/%.c $(DEPS)
	@mkdir -p build
	$(CC) $(CFLAGS) -c -o $@ $<

# debug targets; they assume build/data exists and has a proper config file
run: build/inoue
	build/inoue build/data
gdb: build/inoue
	gdb --args build/inoue build/data

.PHONY: all run gdb
