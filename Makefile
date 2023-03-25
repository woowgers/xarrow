SOURCES = src/main.c src/arrow.c
CFLAGS = -g3 -lX11 -lm

.PHONY: xarrow

all: xarrow

xarrow: $(SOURCES)
	$(CC) -o xarrow $(SOURCES) $(CFLAGS)

local_install: xarrow
	cp -t /usr/local/bin xarrow

install: xarrow
	cp -t /usr/bin xarrow
