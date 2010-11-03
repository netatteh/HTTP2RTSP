#ifndef TIMETEST
#define TIMETEST

#define QUEUESIZE 20 
#define TOTALEVENTS 200 

typedef struct testframe
{
  int timestamp;
  int data;
} TestFrame;



typedef struct timeoutevent
{
  struct timeoutevent *next, *prev;
  struct timeval time;
  TestFrame *frame;
} TimeoutEvent;


typedef struct queue
{
  TimeoutEvent *first, *last;
  int size;
} Queue;


void *thread_routine(void *parameter);

void init_event(TimeoutEvent *event, struct timeval time, int timestamp);

#endif

