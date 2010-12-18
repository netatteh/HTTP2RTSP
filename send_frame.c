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


int send_video_frame(unsigned char *buf, struct frame *myFrame, int sockfd, uint16_t seqnum) {
  const unsigned char firstbyte = FIRSTBYTE;
  unsigned char secondbyte;

  size_t buflen;
  uint8_t *datastep;
  unsigned char *bufstep;
  uint8_t nalbyte = myFrame->data[4];
  uint8_t origtype = nalbyte & 0x1f;
  uint8_t fragtype = 0x1c;
  uint8_t fustart = 0x4, fuend = 0x2;
  int numpkts, remains, spaceForData, firstpacket = 1;

  /* RTP header takes 12 bytes */
  spaceForData = BUFSIZE - 12;

  remains = myFrame->size - 4;
  numpkts = 0;
  datastep = myFrame->data + 4;

  while (remains > 0) {
    
    bzero(buf, BUFSIZE);
    bufstep = buf;

    /* Last packet */
    if (remains <= spaceForData) {
      buflen = remains + 12;
      secondbyte = MARKEDSECONDBYTE_H264;
    }
    /* "Full packet" */
    else {
      buflen = spaceForData + 12;
      secondbyte = UNMARKEDSECONDBYTE_H264;
    }

    memcpy(bufstep, &firstbyte, 1);
    memcpy(bufstep + 1, &secondbyte, 1);
    packi16(bufstep + 2, seqnum++);
    pack32i(bufstep + 4, myFrame->timestamp);
    pack32i(bufstep + 8, ssrc);
    bufstep += 12;

    /* Beginning of copying nals and data */

    /* The first packet of a possibly fragmented unit */
    if (firstpacket) {
      firstpacket = 0;

      /* The packet needs fragmenting */
      if (spaceForData / remains < 1) {
        datastep++;
        *bufstep++ = (nalbyte & 0xe0) | fragtype;
        *bufstep++ = (fustart << 5) | origtype;
        memcpy(bufstep, datastep, spaceForData - 2);
        remains -= spaceForData - 2;
        datastep += spaceForData - 2; 
      }

      /* No fragmenting required */
      else {
        memcpy(bufstep, datastep, remains);
        remains -= remains;
      }
    }

    /* The last packet of a fragmented unit */
    else if (remains <= spaceForData - 2) {
      *bufstep++ = (nalbyte & 0xe0) | fragtype;
      *bufstep++ = (fuend << 5) | origtype;
      memcpy(bufstep, datastep, remains);
      remains -= remains;
    }

    /* An intermediary packet */
    else {
      *bufstep++ = (nalbyte & 0xe0) | fragtype;
      *bufstep++ = origtype;
      memcpy(bufstep, datastep, spaceForData - 2);
      remains -= spaceForData - 2;
      datastep += spaceForData - 2;
    }

    send_all(sockfd, buf, buflen);

    numpkts++;
  }

  return numpkts;

}


int send_audio_frame(unsigned char *buf, struct frame *myframe, int sockfd, uint16_t seqnum)
{
  unsigned char firstbyte = FIRSTBYTE;
  unsigned char secondbyte = SECONDBYTE_PCMA;

  memset(buf, 0, BUFSIZE);

  memcpy(buf, &firstbyte, 1);
  memcpy(buf + 1, &secondbyte, 1);
  packi16(buf + 2, seqnum++);
  pack32i(buf + 4, myframe->timestamp);
  pack32i(buf + 8, ssrc);

  memcpy(buf + 12, myframe->data, myframe->size);
  send_all(sockfd, buf, myframe->size + 12);

  return 1;
}


void send_dummy_rtp(unsigned char *sendbuf, int sockfd, uint16_t *seqnum)
{
  unsigned char firstbyte = FIRSTBYTE;
  unsigned char secondbyte = UNMARKEDSECONDBYTE_H264;

  memset(sendbuf, 0, BUFSIZE);
  memcpy(sendbuf, &firstbyte, 1);
  memcpy(sendbuf + 1, &secondbyte, 1);
  packi16(sendbuf + 2, (*seqnum)++);
  pack32i(sendbuf + 4, 0);
  pack32i(sendbuf + 8, ssrc);

  send_all(sockfd, sendbuf, 12);
}














