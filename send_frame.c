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
  const unsigned char firstbyte = FIRSTBYTE;
  unsigned char secondbyte;

  size_t buflen;
  uint8_t *stepper;
  int numpkts, remains, spaceForData;

  /* RTP header takes 12 bytes */
  spaceForData = BUFSIZE - 12;

  remains = myFrame->size;
  numpkts = 0;
  stepper = myFrame->data;

  while (remains > 0) {
    /* Last packet */
    if (remains <= spaceForData) {
      buflen = remains + 12;
      secondbyte = UNMARKEDSECONDBYTE;
    }
    /* "Full packet" */
    else {
      buflen = spaceForData + 12;
      secondbyte = UNMARKEDSECONDBYTE;
    }

    bzero(buf, BUFSIZE);
    memcpy(buf, &firstbyte, 1);
    memcpy(buf + 1, &secondbyte, 1);
    packi16(buf + 2, seqnum++);
    pack32i(buf + 4, myFrame->timestamp);
    pack32i(buf + 8, ssrc);

    /* Last packet */
    if (remains <= spaceForData) {
      memcpy(buf + 12, stepper, remains);
      remains -= remains;
    }
    else {
      memcpy(buf + 12, stepper, spaceForData);
      remains -= spaceForData;
      stepper += spaceForData;
    }

    send_all(sockfd, buf, buflen);

    numpkts++;
  }

  return numpkts;

}

