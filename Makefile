CC = gcc
CFLAGS = -Wall -I.
LDFLAGS = -lncurses -lrt -pthread

all: executables/broker executables/player

executables/broker: broker/broker.c
	$(CC) $(CFLAGS) broker/broker.c -o executables/broker $(LDFLAGS)

executables/player: player/player.c
	$(CC) $(CFLAGS) player/player.c -o executables/player $(LDFLAGS)

clean:
	rm -f executables/*