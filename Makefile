## Compilation
CC = gcc
VERSION = $(shell grep '^version=' metapkg | cut -d= -f2)

TUNDRA_STD = -std=c11
TUNDRA_DEFS = -D_GNU_SOURCE -DTUNDRA_IP_VERSION=\"$(VERSION)\"
TUNDRA_INC = -Isrc/net -Isrc/util -Isrc/cmd

CFLAGS ?= -Wall -Wextra -O2

TARGET = tundra-ip

SRCS = $(wildcard src/*.c src/net/*.c src/util/*.c src/cmd/*.c)
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TUNDRA_STD) $(TUNDRA_INC) $(TUNDRA_DEFS) \
		-MMD -MP -c $< -o $@

$(OBJS): Makefile metapkg

-include $(DEPS)

clean:
	rm -f $(TARGET) $(OBJS) $(DEPS)


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
