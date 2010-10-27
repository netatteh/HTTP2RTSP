#include <stdio.h>
#include <string.h>

#include "httpmsg.h"

/* Small program for unit testing different functionalities of the program
 * separately */

int main()
{
  char *url1 = "http://www.netlab.tkk.fi/~varun/videos";
  char *url2 = "www.netlab.tkk.fi/~varun/videos";
  char *test1 = "Transport: RTP/AVP;unicast;client_port=40404-40405";
  unsigned char buffer[1500];
  int i, j;
  RTSPMsg msg;

  sscanf(test1, "client_port=%d-%d", &i, &j);
  printf("\n%d %d\n", i, j);

  http_get(url1, buffer);
  printf("url1:\n%s\n", buffer);

  http_get(url2, buffer);
  printf("url2:\n%s\n", buffer);
  memset(&msg, 0, sizeof(msg));
  msg.fields |= (F_TYPE | F_CSEQ | F_SESSION | F_CONTTYPE | F_CONTLEN);
  msg.type = DESCRIBE;
  msg.cseq = 27;
  msg.session = 12345;
  msg.contentlen = 97;
  strncpy(msg.contenttype, "application/sdp", FIELDLEN);

  write_rtsp(&msg, buffer);
  printf("%s", buffer);
  parse_rtsp(&msg, buffer);
  write_rtsp(&msg, buffer);
  printf("%s", buffer);

  msg.fields |= (F_TYPE | F_CSEQ);
  msg.type = DESCRIBE;
  rtsp_describe(&msg, buffer);
  printf("%s", buffer);

  return 0;
}
