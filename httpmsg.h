#ifndef HTTP2RTSP_HTTP_MSG_H
#define HTTP2RTSP_HTTP_MSG_H

#define URLSIZE 500
#define FIELDLEN 100
#define PORTLEN 10

struct client;

enum rtspmsgtypes
{
  OPTIONS,
  DESCRIBE,
  SETUP,
  PLAY,
  TEARDOWN,
  OK
};

enum rtspfields
{
  F_TYPE = 0x1,
  F_CSEQ = 0x2,
  F_SESSION = 0x4,
  F_CONTLEN = 0x8,
  F_CONTTYPE = 0x10,
  F_TRANSPORT = 0x20,
  F_RANGE = 0x40,
  F_DATE = 0x80,
  F_DATA = 0x100
};


typedef struct rtspmsg
{
  int fields;
  int type;
  int cseq;
  int session;
  int contentlen;
  int playstart, playend;
  char transport[FIELDLEN];
  char clirtpport[PORTLEN];
  char clirtcpport[PORTLEN];
  char date[FIELDLEN];
  char contenttype[FIELDLEN];
  char data[URLSIZE];
  char range[FIELDLEN];
} RTSPMsg;

/* Builds a HTTP/1.1 GET message for retrieving the resource
 * denoted by "url" and stores it in "buffer". Returns the total
 * length of the generated message. */
int http_get(const char *url, unsigned char *buffer);


/* Assigns the host part of the parameter "url" to "host", and path part
 * to "path". Removes the "http://" prefix if necessary. Assumes that
 * "host" and "path" have sufficient room to hold the resulting strings. */
void parse_url(const char *url, char *host, char *path);


/* Parses an incoming RTSP message from the buffer, and saves the data
 * to a RTSPMsg struct */
int parse_rtsp(RTSPMsg *msg, const unsigned char *buffer);

/* Writes out the contents of an RTSPMsg structure in a correct form
 * to the given buffer. For now assumes that an RTSP/1.0 200 OK is sent */
int write_rtsp(const RTSPMsg *msg, unsigned char *buffer);

int rtsp_options(const RTSPMsg *msg, struct client *client, unsigned char *buf);

int rtsp_describe(struct client *client, unsigned char *buf);

int rtsp_setup(const RTSPMsg *msg, struct client *client, unsigned char *buf, int rtp, int rtcp);

int rtsp_play(const RTSPMsg *msg, unsigned char *buf);

int rtsp_ping(unsigned char *buf, int session);

int rtsp_teardown(const RTSPMsg *msg, unsigned char *buf);

#endif
