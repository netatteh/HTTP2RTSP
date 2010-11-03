#include "parse_video.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
  AVFormatContext *ctx;
  int videoIdx, audioIdx, temp, i;
  double audioRate, videoRate;
  int vid, aud, a;
  uint8_t *data;
  struct frame *myFrame = malloc(sizeof(struct frame));
  int vid, aud;
  size_t a;
  uint8_t *data;

  if (argc != 2) {
    printf("Usage: ./%s filename\n", argv[0]);
    return 0;
  }

  initialize_context(&ctx, argv[1], &videoIdx, &audioIdx, &videoRate, &audioRate);

  printf("%i video: frame rate %f fps\n", videoIdx, videoRate);
  printf("%i audio: sample rate %f\n", audioIdx, audioRate);

  vid = aud = a = 0;
  for (i=0; i<100; i++) {   
    temp = get_frame(ctx, myFrame, videoIdx, audioIdx, videoRate, audioRate);

    if (temp == videoIdx) {
      vid++;
    }

    else if (temp == audioIdx) {
      printf("Audio %i, timestamp %i, size %i\n", aud, myFrame->timestamp, myFrame->size);
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

  return 0;
}


