CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -D_GNU_SOURCE
TARGET = tundra-ip
OBJS = main.o netlink.o link.o

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

main.o: main.c netlink.h
	$(CC) $(CFLAGS) -c main.c

netlink.o: netlink.c netlink.h
	$(CC) $(CFLAGS) -c netlink.c

link.o: link.c netlink.h
	$(CC) $(CFLAGS) -c link.c

clean:
	rm -f $(TARGET) $(OBJS)
