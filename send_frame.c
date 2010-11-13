#include <stdlib.h>

#include "send_frame.h"
#include "socketfunc.h"
#include <arpa/inet.h>

uint32_t ssrc = 8472857;

/* Packs an unsigned short (2bytes) to char buffer */
/* Model from Beej's Guide */
void packi16(unsigned char *buf, unsigned short i) {
  *buf++ = i>>8;
  *buf++ = i;
}


void pack32i(unsigned char *buf, uint32_t src)
{
  int i;
  for (i = 24; i >= 0; i -= 8) {
    *buf++ = src >> i;
  }
}


int send_frame(unsigned char *buf, struct frame *myFrame, int sockfd, uint16_t seqnum) {
  const unsigned char firstbyte = 0x80;
  const unsigned char secondbyte = 0x60;

  size_t buflen;
  int numpkts, remains, spaceForData;

  /* RTP header takes 12 bytes */
  spaceForData = BUFSIZE - 12;

  remains = myFrame->size;
  numpkts = 0;

  while (remains > 0) {
    /* Last packet */
    if (remains <= spaceForData) {
      buflen = remains + 12;
    }
    /* "Full packet" */
    else {
      buflen = spaceForData + 12;
    }

    bzero(buf, BUFSIZE);
    memcpy(buf, &firstbyte, 1);
    memcpy(buf + 1, &secondbyte, 1);
    packi16(buf + 2, seqnum++);
    pack32i(buf + 4, myFrame->timestamp);
    pack32i(buf + 8, ssrc);
    /*sprintf((char*)(buf + 4), "%u", htonl(myFrame->timestamp));
    sprintf((char*)(buf + 8), "%u", htonl(ssrc)); */

    /* Last packet */
    if (remains <= spaceForData) {
      memcpy(buf + 12, myFrame->data, remains);
      remains -= remains;
    }
    else {
      memcpy(buf + 12, myFrame->data, spaceForData);
      remains -= spaceForData;
    }

    send_all(sockfd, buf, buflen);

    numpkts++;
  }

  return numpkts;

}

