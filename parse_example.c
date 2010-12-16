#include "parse_video.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <glib.h>
#include <glib/gprintf.h>




int main(int argc, char *argv[]) {
  /*
  AVFormatContext *ctx;
  int videoIdx, audioIdx, temp, i;
  double audioRate, videoRate;
  int vid, aud;
  size_t a;
  uint8_t *data;
  int fd;
  char sdpbuf[2048];

  if (argc != 2) {
    printf("Usage: ./%s filename\n", argv[0]);
    return 0;
  }


  initialize_context(&ctx, argv[1], &videoIdx, &audioIdx, &videoRate, &audioRate);

  printf("%i video: frame rate %f fps\n", videoIdx, videoRate);
  printf("%i audio: sample rate %f\n", audioIdx, audioRate);

  if (avf_sdp_create(&ctx, 1, sdpbuf, sizeof(sdpbuf)) != 0) {
    printf("Error creating the sdp!\n");
    exit(1);
  }

  printf("%s\n", sdpbuf);

  vid = aud = a = 0;
  for (i=0; i<100; i++) {   
    temp = get_frame(ctx, myFrame, videoIdx, audioIdx, videoRate, audioRate);
    if (temp == -1) break;

    if (temp == videoIdx) {
      vid++;
      printf("Video %i, timestamp %i, size %i\n", vid, myFrame->timestamp, (int)myFrame->size);
      data = myFrame->data;
      printf("data: ");
      for (a=0; a<myFrame->size; a++) {
        printf("%x:", *data);
        data++;
      }
      printf("\n");
    }

    else if (temp == audioIdx) {
      printf("Audio %i, timestamp %i, size %i\n", aud, myFrame->timestamp, (int)myFrame->size);
      data = myFrame->data;
      printf("data: ");
      for (a=0; a<myFrame->size; a++) {
        printf("%i:", *data);
        data++;
      }
      printf("\n");
      aud++;
    }
    free(myFrame->data);

  }

  free(myFrame);


  close_context(ctx);
  */

  return 0;
}


