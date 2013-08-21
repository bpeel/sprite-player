OBJS = sprite-player.o
CFLAGS = `pkg-config cogl2 cogl-gst --cflags` -g -Wall
LDFLAGS = `pkg-config cogl2 cogl-gst --libs`
CC = gcc

all : sprite-player

sprite-player : $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

.c.o :
	$(CC) -c $(CFLAGS) -o $@ $<


clean :
	rm -f sprite-player *.o

.PHONY : clean all
