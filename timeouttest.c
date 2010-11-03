#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <search.h>
#include <pthread.h>

#include "timeouttest.h"
#include "util.h"
#include "fileio.h"

pthread_mutex_t queuelock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queuecond = PTHREAD_COND_INITIALIZER;


Queue queue;


int main()
{
  pthread_t filler;      
  TimeoutEvent *tempevent;
  int i = TOTALEVENTS;

  queue.first = queue.last = NULL;
  queue.size = 0;

  CHECK((pthread_create(&filler, NULL, thread_routine, NULL)) == 0);
  pthread_detach(filler);

  while (i > 0) {

    lock_mutex(&queuelock);

    if (queue.first) {
      printf("Time: %d, Data: %d\n", queue.first->frame->timestamp, queue.first->frame->data);
      tempevent = queue.first;
      queue.first = queue.first->next;
      remque(tempevent);
      free(tempevent->frame);
      free(tempevent);
      queue.size--;
      i--;

      if (queue.size < QUEUESIZE / 2) {
        pthread_cond_signal(&queuecond);
      }

      unlock_mutex(&queuelock);
      usleep(10000);
    }
    else {
      pthread_cond_signal(&queuecond);
      unlock_mutex(&queuelock);
      sleep(1);
    } 

  }

  return 0;
}


void *thread_routine(void *parameter)
{
  int i = TOTALEVENTS;
  TimeoutEvent *event = NULL;
  struct timeval time;

  while (i > 0) {

    lock_mutex(&queuelock);

    if (queue.size < QUEUESIZE) {

      pthread_cond_wait(&queuecond, &queuelock);

      event = (TimeoutEvent *)malloc(sizeof(TimeoutEvent)); 
      gettimeofday(&time, NULL);
      time.tv_sec += 10;
      init_event(event, time, i);

      if (!queue.first) {
        queue.first = queue.last = event;
      }
      else {
        insque(event, queue.last);
        queue.last = event;
      }

      queue.size++;
      i--;
      unlock_mutex(&queuelock);
    }
    else {
      unlock_mutex(&queuelock);
      usleep(1);
    }

  }

  pthread_exit(NULL);
}


void init_event(TimeoutEvent *event, struct timeval time, int timestamp)
{
  event->next = event->prev = NULL;
  event->time = time;
  event->frame = (TestFrame *)malloc(sizeof(TestFrame));
  event->frame->timestamp = timestamp;
  event->frame->data = (rand() % 100) + 1;
}


