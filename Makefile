CC=gcc
CFLAGS=-g3 -Wall -Wextra -pedantic
LDFLAGS=-lavformat -lavcodec

SUFFIXES=.c .o

PROGS=http2rtsp tester parsetest

all: $(PROGS) cleano

.c.o:	
	$(CC) $(CFLAGS) -c $<

http2rtsp: main.o fileio.o socketfunc.o util.o httpmsg.o server.o parse_video.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

tester: tester.o httpmsg.o

parsetest: parse_video.o parse_example.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@
	
clean:
	rm -f *.o $(PROGS) gmon.out

cleano:
	rm -f *.o

