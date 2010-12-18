CC=gcc `pkg-config --libs --cflags glib-2.0`
CFLAGS=-g3 -Wall -Wextra -pedantic
LDFLAGS=-lavformat -lavcodec 

SUFFIXES=.c .o

PROGS=http2rtsp tester parsetest timeouttest rtptest

all: $(PROGS)

.c.o:	
	$(CC) $(CFLAGS) -c $<

http2rtsp: main.o fileio.o socketfunc.o util.o httpmsg.o server.o parse_video.o send_frame.o mt_server.o sipfunctions.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

tester: tester.o httpmsg.o

parsetest: parse_video.o parse_example.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

timeouttest: timeouttest.o util.o fileio.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

rtptest: send_frame.o rtptest.o fileio.o socketfunc.o util.o parse_video.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

clean:
	rm -f *.o $(PROGS) gmon.out

cleano:
	rm -f *.o

