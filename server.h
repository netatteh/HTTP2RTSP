#ifndef HTTP2RTSP_SERVER_H
#define HTTP2RTSP_SERVER_H

#include "parse_video.h"


#define QUEUESIZE 100

enum mediastates
{
  IDLE,
  GETSENT,
  RECVTCP,
  STREAM
};

enum clientstates
{
  NOCLIENT,
  CLICONNECTED,
  SDPSENT,
  SETUPSENT,
  STREAMING
};

typedef struct client
{
  int state;
  int rtspfd;
  int videofds[2];
  int audiofds[2];
} Client;


typedef struct timeoutevent
{
  struct timeoutevent *next, *prev;
  struct timeval time;
  Frame *frame;
} TimeoutEvent;


typedef struct queue
{
  TimeoutEvent *first, *last;
  int size;
  int sendrate;
} Queue;

void init_client(Client *client);

void *fill_queue(void *init);

int start_server(const char *url, const char *rtspport);

/* Inserts the given TimeoutEvent to the give queue. Retains the order
 * of the queue, smallest timestamp first. */
void push_event(TimeoutEvent *event, Queue *queue);

/* Pulls the first item from the given queue. If the queue is empty, returns NULL */
TimeoutEvent *pull_event(Queue *queue);

void set_timeval(TimeoutEvent *event);


#endif

