#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <search.h>

#include "server.h"
#include "httpmsg.h"
#include "socketfunc.h"
#include "fileio.h"
#include "util.h"
#include "parse_video.h"
#include "send_frame.h"


/* Defined in main.c, used for logging */
extern int logfd;


/* A mutex and a conditional variable used when reading of modifying
 * the queue of frames */
pthread_mutex_t queuelock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queuecond = PTHREAD_COND_INITIALIZER;

/* The queue used in sending packets */
Queue queue;

void init_client(Client *client)
{
  client->state = NOCLIENT;
  client->rtspfd = -1;
  client->videofds[0] = -1;
  client->videofds[1] = -1;
  client->audiofds[0] = -1;
  client->audiofds[1] = -1;
}


void push_event(TimeoutEvent *event, Queue *queue)
{
  TimeoutEvent *stepper = queue->last;

  printf("Pushing to queue...\n");
  fflush(stdout);
  if (stepper == NULL) {
    queue->first = queue->last = event;
  }
  else {
    while (stepper->frame->timestamp > event->frame->timestamp && stepper != NULL) {
      printf("Queue %d > Event %d\n", stepper->frame->timestamp, event->frame->timestamp);
      stepper = stepper->prev;
    }
    if (stepper == NULL) {
      queue->first->prev = event;
      event->next = queue->first;
      queue->first = event;
    }
    else {
      insque(event, stepper);
      if (event->next == NULL) {
        queue->last = event;
      }
    }
  }

  queue->size++;
}


TimeoutEvent *pull_event(Queue *queue)
{
  TimeoutEvent *ret = queue->first;

  printf("Pulling from queue...\n");
  fflush(stdout);

  if (ret != NULL) {
    queue->first = queue->first->next;  
    remque(ret);
    queue->size--;
  }

  return ret;
}


void set_timeval(TimeoutEvent *event)
{
  event->time.tv_sec = event->frame->timestamp / 90000;
  event->time.tv_usec = (event->frame->timestamp % 90000) / 0.09;
}

struct timeval caclulate_delta(struct timeval *first, struct timeval *second)
{
  struct timeval delta;

  delta.tv_sec = second->tv_sec - first->tv_sec;
  delta.tv_usec = second->tv_usec - first->tv_usec; 

  if (delta.tv_usec < 0) {
    delta.tv_usec += 1000000;
  }

  printf("Delta: %ld sec, %ld usec\n", delta.tv_sec, delta.tv_usec);

  return delta;
}

void *fill_queue(void *fname)
{
  AVFormatContext *ctx;
  int videoIdx, audioIdx;
  double videoRate, audioRate;
  char *filename = (char *)fname;
  int frametype;
  int quitflag = 0, mutlocked = 0;
  Frame *frame;
  TimeoutEvent *event;

  initialize_context(&ctx, "videotemp.mp4", &videoIdx, &audioIdx, &videoRate, &audioRate);

  lock_mutex(&queuelock);
  queue.sendrate = ctx->bit_rate;

  while (!quitflag) {

    mutlocked = 1;
    pthread_cond_wait(&queuecond, &queuelock);

    while (queue.size < QUEUESIZE) {

      if (!mutlocked) {
        lock_mutex(&queuelock);  
      }

      frame = (Frame *)malloc(sizeof(Frame));

      /* Get the frame. If none are available, end the loop and the entire function. */
      if ((frametype = get_frame(ctx, frame, videoIdx, audioIdx, videoRate, audioRate)) == -1) {
        quitflag = 1;
        break;
      }
      frame->frametype = (frametype == videoIdx)?VIDEO_FRAME:AUDIO_FRAME;

      event = (TimeoutEvent *)malloc(sizeof(TimeoutEvent));
      event->frame = frame;
      set_timeval(event);
      event->next = event->prev = NULL;

      push_event(event, &queue);

      unlock_mutex(&queuelock);
      mutlocked = 0;
      usleep(10000);


    } /* End of inner while loop */

  } /* End of outer while loop */
  
  pthread_exit(NULL);
}


int start_server(const char *url, const char *rtspport)
{
  int mediafd = -1, listenfd, tempfd, maxfd, sendfd;
  int videofd;
  struct addrinfo *info;
  struct sockaddr_storage remoteaddr;
  socklen_t addrlen = sizeof remoteaddr;
  fd_set readfds, masterfds;
  struct timeval *timeout, *timeind = NULL;
  int nready, i;
  int videosize, videoleft;
  int recvd, sent;
  char urlhost[URLSIZE], urlpath[URLSIZE], tempstr[URLSIZE];
  unsigned char msgbuf[BUFSIZE], sendbuf[BUFSIZE];
  char *temp;
  RTSPMsg rtspmsg;
  Client streamclient;
  pthread_t threadid;

  int rtpseqno = (rand() % 1000000);
  TimeoutEvent *event;

  /* The current state of the protocol */
  int mediastate = IDLE;
  int quit = 0;

  timeout = (struct timeval *)malloc(sizeof(struct timeval));
  
  init_client(&streamclient);

  /* Open the a file where the video is to be stored */
  if ((videofd = open("videotemp.mp4", O_RDWR | O_CREAT | O_TRUNC, S_IRWXU)) < 0) {
    fatal_error("Error opening the temporary videofile");
  }

  /* Create the RTSP listening socket */
  resolve_host(NULL, rtspport, SOCK_STREAM, AI_PASSIVE, &info);
  listenfd = server_socket(info);
  maxfd = listenfd;


  FD_ZERO(&readfds);
  FD_ZERO(&masterfds);
  FD_SET(listenfd, &masterfds);


  while (!quit) {

    readfds = masterfds;

    if ((nready = Select(maxfd + 1, &readfds, timeind)) == -1) {
      write_log(logfd, "Select interrupted by a signal\n");
    } 

    /* Timeout handling, used for packet pacing */
    else if (nready == 0) {
      timeind = NULL;
      lock_mutex(&queuelock);
      if ((event = pull_event(&queue)) != NULL) {

        /* TODO: For now only video frames are sent */
        if (event->frame->frametype == VIDEO_FRAME) {
          send_frame(sendbuf, event->frame, streamclient.videofds[0], rtpseqno++);
        }

        /* If there are elements left in the queue, calculate next timeout */
        if (queue.first != NULL) {
          *timeout = caclulate_delta(&event->time, &queue.first->time);
          timeind = timeout;
        }

        free(event->frame->data);
        free(event->frame);
        free(event);
      }
      unlock_mutex(&queuelock);
      continue;
    } /* End of timeout handling */

    /* Start to loop through the file descriptors */
    for (i = 0; i <= maxfd; i++) {
      if (FD_ISSET(i, &readfds)) {

        nready--;

        /* New connection from a client */
        if (i == listenfd) {
          if ((tempfd = accept(i, (struct sockaddr *)&remoteaddr, &addrlen)) == -1) {
            if (errno != EWOULDBLOCK && errno != ECONNABORTED &&
                errno != EPROTO && errno != EINTR)
            {
              fatal_error("accept");
            }
          }

          /* If we are already serving a client, close the new connection. Otherwise, continue. */
          if (streamclient.state != NOCLIENT) close (tempfd);
          else {
            streamclient.rtspfd = tempfd;
            streamclient.state = CLICONNECTED;
            maxfd = max(2, streamclient.rtspfd, maxfd);
            FD_SET(streamclient.rtspfd, &masterfds);
          }
        }

        /* Data from the media source */
        else if (i == mediafd) {

          switch (mediastate) {

            case GETSENT:
              /* Read ONLY the HTTP message from the socket and store the video size */
              recvd = recv_all(i, msgbuf, BUFSIZE, MSG_PEEK);
              temp = strstr((char *)msgbuf, "\r\n\r\n");
              recvd = recv_all(i, msgbuf, (int)(temp + 4 - (char *)msgbuf), 0);
              temp = strstr((char *)msgbuf, "Content-Length:");
              sscanf(temp, "Content-Length: %d", &videosize);
              videoleft = videosize;
              mediastate = RECVTCP;
              break;

            case RECVTCP:
              if ((recvd = recv_all(i, msgbuf, BUFSIZE, 0)) == 0) {
                FD_CLR(i, &masterfds);
                close(i);
                printf("Socket closed\n");
              }
              writestr(videofd, msgbuf, recvd);
              videoleft -= recvd;
              if (videoleft <= 0) {
                close(videofd);
                CHECK((pthread_create(&threadid, NULL, fill_queue, NULL)) == 0);
                pthread_detach(threadid);
                sleep(1);
                pthread_cond_signal(&queuecond);
                mediastate = STREAM;
                timeout->tv_sec = 1;
                timeout->tv_usec = 0;
                timeind = timeout;
              }
              break;

              /* TODO: Start streaming, currently just exits the program */
            case STREAM:
              /*
                 close(videofd);
                 close(mediafd);
                 close(listenfd);
                 quit = 1;
                 */
              break;

            default: 
              break;
          }
        }

        /* Data from a client ( i == streamclient.rtspfd) */
        else {

          if ((recvd = recv_all(i, msgbuf, BUFSIZE, 0)) == 0) {
            FD_CLR(i, &masterfds);
            close(i);
            printf("Socket closed\n");
            streamclient.state = NOCLIENT;
          }
          else {
            printf("%s", msgbuf);
            parse_rtsp(&rtspmsg, msgbuf); 
          }

          switch (streamclient.state) {

            case CLICONNECTED:
              if (rtspmsg.type == OPTIONS) {
                sent = rtsp_options(&rtspmsg, sendbuf);
                send_all(i, sendbuf, sent);
              }
              else if (rtspmsg.type == DESCRIBE) {

                /* Start fetching the file from the server */
                parse_url(url, urlhost, urlpath);
                resolve_host(urlhost, "80", SOCK_STREAM, 0, &info);
                mediafd = client_socket(info, 0);
                FD_SET(mediafd, &masterfds);
                maxfd = max(2, maxfd, mediafd);

                /* Send the GET message */
                http_get(url, msgbuf);
                send_all(mediafd, msgbuf, strlen((char *)msgbuf));
                mediastate = GETSENT;

                /* TODO: parse SDP from the media file rather than hardcoding it */
                sent = rtsp_describe(&rtspmsg, sendbuf);
                send_all(i, sendbuf, sent);
                streamclient.state = SDPSENT;
              }
              break;

            case SDPSENT:
              if (rtspmsg.type == SETUP) {
                sent = rtsp_setup(&rtspmsg, sendbuf, 50508, 50509);
                send_all(i, sendbuf, sent);
                write_remote_ip(tempstr, streamclient.rtspfd);
                resolve_host(tempstr, rtspmsg.clirtpport, 0, SOCK_DGRAM, &info); 
                streamclient.videofds[0] = client_socket(info, 50508);
                resolve_host(tempstr, rtspmsg.clirtcpport, 0, SOCK_DGRAM, &info);
                streamclient.videofds[1] = client_socket(info, 50509);
                streamclient.state = SETUPSENT;
              }
              break;

            case SETUPSENT:
              if (rtspmsg.type == PLAY) {
                sent = rtsp_play(&rtspmsg, sendbuf);
                send_all(i, sendbuf, sent);
                streamclient.state = STREAMING;
              }
              
              break;

            default:
              break;
          }
        }
      }
      if (nready <= 0) break;   
    }

  }


  return 1;
}


