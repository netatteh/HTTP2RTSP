#ifndef SEND_FRAME_H
#define SEND_FRAME_H

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include "parse_video.h"   // contains definition of struct frame

/* Packs frame into one or more RTP packets and sends to sockd */
/* Returns the number of packets that were sent */
int send_frame(struct frame *myframe, int sockfd, uint16_t seqnum);

#endif
