#include <stdlib.h>

#include "send_frame.h"
#include "socketfunc.h"

uint32_t ssrc = 8472857;

int send_frame(struct frame *myFrame, int sockfd, uint16_t seqnum) {
  const unsigned char firstbyte = 0x80;
  const unsigned char secondbyte = 0x60;
  unsigned char *buf;
  size_t buflen;
  int numpkts, remains, spaceForData;

  // RTP header takes 12 bytes
  spaceForData = BUFSIZE - 12;

  remains = myFrame->size;
  numpkts = 0;

  while (remains > 0) {
    // Last packet
    if (remains <= spaceForData) {
      buflen = remains + 12;
    }
    // "Full packet"
    else {
      buflen = spaceForData + 12;
    }
    
    buf = malloc(buflen);
    bzero(buf, buflen);
    memcpy(buf, &firstbyte, 1);
    memcpy(buf + 1, &secondbyte, 1);
    sprintf((char*)(buf + 2), "%hu", seqnum);
    sprintf((char*)(buf + 4), "%u", myFrame->timestamp + numpkts);
    sprintf((char*)(buf + 8), "%u", ssrc);
    
    // Last pakcet
    if (remains <= spaceForData) {
      memcpy(buf + 12, myFrame->data, remains);
      remains -= remains;
    }
    else {
      memcpy(buf + 12, myFrame->data, spaceForData);
      remains -= spaceForData;
    }
    
    send_all(sockfd, buf, buflen);
    free(buf);

    numpkts++;
  }

  return numpkts;
}
