#ifndef HTTP2RTSP_SERVER_H
#define HTTP2RTSP_SERVER_H

#include "parse_video.h"

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
  struct timeval time;
  int state;
  Frame *frame;
} TimeoutEvent;


void init_client(Client *client);

int start_server(const char *url, const char *rtspport);


#endif

