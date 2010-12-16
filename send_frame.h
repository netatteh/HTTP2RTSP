#ifndef SEND_FRAME_H
#define SEND_FRAME_H

#include "parse_video.h"   /* definition of struct frame */

#define FIRSTBYTE 0x80
#define MARKEDSECONDBYTE 0xe0
#define UNMARKEDSECONDBYTE 0x60

/* Packs frame into one or more RTP packets and sends to sockd
 * Returns the number of packets that were sent */
int send_frame(unsigned char *buf, struct frame *myframe, int sockfd, uint16_t seqnum);

/* Sends an rtp packet with a correct format and an empty payload. 
 * Used to delay the timeout at PLAY stage if still downloading a file. */
void send_dummy_rtp(unsigned char *sendbuf, int sockfd, uint16_t *seqnum);

#endif
