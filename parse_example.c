#include "parse_video.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
  AVFormatContext *ctx;
  int videoIdx, audioIdx, temp, i;
  double audioRate, videoRate;
  struct frame *myFrame = malloc(sizeof(struct frame));

  initialize_context(&ctx, argv[1], &videoIdx, &audioIdx, &videoRate, &audioRate);

  printf("%i video: frame rate %f fps\n", videoIdx, videoRate);
  printf("%i audio: sample rate %f\n", audioIdx, audioRate);

  int vid, aud, a;
  vid = aud = a = 0;
  //while ( (temp = get_frame(ctx, myFrame, videoIdx, audioIdx, videoRate, audioRate)) >= 0) {
  for (i=0; i<100; i++) {   
    temp = get_frame(ctx, myFrame, videoIdx, audioIdx, videoRate, audioRate);
 
    if (temp == videoIdx) {
      // tittidii
      //printf("Video %i, timestamp %i, size %i\n", vid, myFrame->timestamp, myFrame->size);
      vid++;
    }

    else if (temp == audioIdx) {
      // tattadaa
      printf("Audio %i, timestamp %i, size %i\n", aud, myFrame->timestamp, myFrame->size);
      uint8_t *data = myFrame->data;
      printf("data: ");
      for (a=0; a<myFrame->size; a++) {
	printf("%i:", *data);
	data++;
      }
      printf("\n");

      aud++;
   }

  }

  close_context(ctx);

  return 0;
}


