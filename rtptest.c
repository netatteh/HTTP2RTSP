#include <stdio.h>
#include "send_frame.h"
#include "parse_video.h"

int logfd;

int main(int argc, char *argv[]) {
  /*
  AVFormatContext *ctx;
  int videoidx, audioidx, i, sockfd, temp;
  uint16_t seqnum;
  double videorate, audiorate;
  struct frame *myFrame = malloc(sizeof(struct frame));
  unsigned char *buf = malloc(1200);

  logfd = 0;

  int send_frame(unsigned char *buf, struct frame *myframe, int sockfd, uint16_t seqnum);
  sockfd = 0;

  if (argc != 2) {
    printf("Usage: ./%s filename\n", argv[0]);
    return 0;
  }

  initialize_context(&ctx, argv[1], &videoidx, &audioidx, &videorate, &audiorate);

  seqnum = 0;

  for (i=0; i<10; i++) {
    get_frame(ctx, myFrame, videoidx, audioidx, videorate, audiorate);
    temp = send_frame(buf, myFrame, sockfd, seqnum);

    printf("Frame %d,  size %d sent in %d packets, seqnum=%d\n", i, (int)myFrame->size, temp, seqnum);
    seqnum += temp;

    free(myFrame->data);
  }

  close_context(ctx);

  free(myFrame);
  free(buf);
  */
  return 0;
}
