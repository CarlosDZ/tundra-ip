CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -D_GNU_SOURCE
TARGET = tundra-ip
OBJS = main.o netlink.o link.o addr.o route.o color.o iface.o

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

main.o: main.c netlink.h
	$(CC) $(CFLAGS) -c main.c

netlink.o: netlink.c netlink.h
	$(CC) $(CFLAGS) -c netlink.c

link.o: link.c netlink.h
	$(CC) $(CFLAGS) -c link.c

addr.o: addr.c netlink.h
	$(CC) $(CFLAGS) -c addr.c

route.o: route.c netlink.h
	$(CC) $(CFLAGS) -c route.c

color.o: color.c color.h
	$(CC) $(CFLAGS) -c color.c

iface.o: iface.c iface.h
	$(CC) $(CFLAGS) -c iface.c

clean:
	rm -f $(TARGET) $(OBJS)
