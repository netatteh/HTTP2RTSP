#ifndef SEND_FRAME_H
#define SEND_FRAME_H

#include "parse_video.h"   /* definition of struct frame */

#define FIRSTBYTE 0x80
#define MARKEDSECONDBYTE_H264 0xe0
#define UNMARKEDSECONDBYTE_H264 0x60
#define SECONDBYTE_PCMA 0x08;

/* Packs frame into one or more RTP packets and sends to sockd. Assumes h.264 encoding.
 * Returns the number of packets that were sent */
int send_video_frame(unsigned char *buf, struct frame *myframe, int sockfd, uint16_t seqnum);

/* Sends an audio frame to sockfd, adding proper RTP headers. Assumes PCM A-law encoding. */
int send_audio_frame(unsigned char *buf, struct frame *myframe, int sockfd, uint16_t seqnum);

/* Sends an rtp packet with a correct format and an empty payload. 
 * Used to delay the timeout at PLAY stage if still downloading a file. */
void send_dummy_rtp(unsigned char *sendbuf, int sockfd, uint16_t *seqnum);

#endif
