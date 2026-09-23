## Compilation

CC = gcc
VERSION = $(shell grep '^version=' metapkg | cut -d= -f2)

# defines e includes propios, SIEMPRE aplicados (fuera de CFLAGS)
TUNDRA_DEFS = -DTUNDRA_IP_VERSION=\"$(VERSION)\"
TUNDRA_INC = -Isrc/net -Isrc/util -Isrc/cmd

CFLAGS ?= -Wall -Wextra -std=c11 -D_GNU_SOURCE

TARGET = tundra-ip

SRCS = $(wildcard src/*.c src/net/*.c src/util/*.c src/cmd/*.c)
OBJS = $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) $(TUNDRA_INC) $(TUNDRA_DEFS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)


## Instalation

PREFIX ?= /usr
DESTDIR ?=

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	install -Dm644 LICENSE $(DESTDIR)$(PREFIX)/share/licenses/$(TARGET)/LICENSE

install-ip: install
	ln -sf $(TARGET) $(DESTDIR)$(PREFIX)/bin/ip

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	rm -f $(DESTDIR)$(PREFIX)/bin/ip
	rm -rf $(DESTDIR)$(PREFIX)/share/licenses/$(TARGET)

.PHONY: install install-ip uninstall clean
