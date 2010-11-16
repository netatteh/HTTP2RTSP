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
  uint8_t *datastep;
  uint8_t nalbyte = 0x1;
  unsigned char *bufstep = buf;
  int numpkts, remains, spaceForData;

  /* RTP header takes 12 bytes */
  spaceForData = BUFSIZE - 12;

  remains = myFrame->size;
  numpkts = 0;
  datastep = myFrame->data;

  while (remains > 0) {
    
    bzero(buf, BUFSIZE);

    /* Last packet */
    if (remains <= spaceForData) {
      buflen = remains + 12;
      secondbyte = MARKEDSECONDBYTE;
    }
    /* "Full packet" */
    else {
      buflen = spaceForData + 12;
      secondbyte = UNMARKEDSECONDBYTE;
    }

    memcpy(buf, &firstbyte, 1);
    memcpy(buf + 1, &secondbyte, 1);
    packi16(buf + 2, seqnum++);
    pack32i(buf + 4, myFrame->timestamp);
    pack32i(buf + 8, ssrc);

    /* Last packet */
    if (remains <= spaceForData) {
      memcpy(bufstep + 12, datastep, remains);
      remains -= remains;
    }
    else {
      memcpy(buf + 12, datastep, spaceForData);
      remains -= spaceForData;
      datastep += spaceForData;
    }

    send_all(sockfd, buf, buflen);

    numpkts++;
  }

  return numpkts;

}

