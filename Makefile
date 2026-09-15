CC = gcc
VERSION = $(shell grep '^version=' metapkg | cut -d= -f2)
CFLAGS = -Wall -Wextra -std=c11 -D_GNU_SOURCE -Isrc/net -Isrc/util -Isrc/cmd -DTUNDRA_IP_VERSION=\"$(VERSION)\"
TARGET = tundra-ip

SRCS = $(wildcard src/*.c src/net/*.c src/util/*.c src/cmd/*.c)
OBJS = $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: .%c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)
