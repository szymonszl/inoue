OBJS = build/inoue.o build/buffer.o build/util.o
DEPS = src/inoue.h src/json.h
CFLAGS = -Wall -g `pkg-config --cflags libcurl`
LDFLAGS = `pkg-config --libs libcurl`

all: build/inoue

build/inoue: $(OBJS)
	@mkdir -p build
	$(CC) $(LDFLAGS) -o $@ $^

$(OBJS): build/%.o: src/%.c $(DEPS)
	@mkdir -p build
	$(CC) $(CFLAGS) -c -o $@ $<

# debug targets; they assume build/data exists and has a proper config file
run: build/inoue
	build/inoue build/data
gdb: build/inoue
	gdb --args build/inoue build/data

.PHONY: all run gdb