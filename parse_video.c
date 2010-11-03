#include <stdio.h>
#include "parse_video.h"

/* Call this first
Initializes AVFormatContext
Returns number of streams in file, -1 if there's an error */
int initialize_context(AVFormatContext **ctx, char *filename, int *videoIdx, int *audioIdx, 
		       double *videoRate, double *audioRate) {
  int videoStream, audioStream;
  size_t i;

  av_register_all();
  if (av_open_input_file(ctx, filename, NULL, 0, NULL) != 0) {
    fprintf(stderr, "initialize_context: could not open video\n");
    return -1;
  }

  if (av_find_stream_info(*ctx) < 0) {
    fprintf(stderr, "initialize_context: could not find stream info\n");
    return -1;
  }

  dump_format(*ctx, 0, filename, 0);

  videoStream = audioStream = -1;

  for (i=0; i<(*ctx)->nb_streams; i++) {
    if ((*ctx)->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) {
      videoStream = i;
    }
    if ((*ctx)->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO) {
      audioStream = i;
    }
  }
  *videoIdx = videoStream;
  *audioIdx = audioStream;

  /* Check frame rate */
  if (videoStream != -1) {
    AVCodecContext *cod = (*ctx)->streams[videoStream]->codec;
    /* printf("Video frame rate:%i.%i\n", cod->time_base.num, cod->time_base.den);
       printf("Video samples per second: %i\n", cod->sample_rate); */

    *videoRate = 0.5 * (double)cod->time_base.den / (double)cod->time_base.num;
  }
  else {
    *videoRate = -1;
  }

  if (audioStream != -1) {
    AVCodecContext *cod = (*ctx)->streams[audioStream]->codec;
    /* printf("Audio frame rate:%i.%i\n", cod->time_base.num, cod->time_base.den); 
       printf("Audio samples per second: %i\n", cod->sample_rate); */

    *audioRate = (double)cod->sample_rate;
  }

  return (*ctx)->nb_streams;
}

/* Call this to get the frames. Allocate yourself memory to myFrame
// Returns videoIdx/audioIdx, indicating to which stream this frame belongs to
// Returns negative number if all frames are read (or error) */
int get_frame(AVFormatContext *ctx, struct frame *myFrame, int videoIdx, int audioIdx, 
	      double videoRate, double audioRate) {
  AVPacket packet;
  int ret;
  if (av_read_frame(ctx, &packet) >= 0) {
    uint8_t *copy = malloc(packet.size*sizeof(uint8_t));
    memcpy(copy, packet.data, packet.size);

    myFrame->data = copy;
    myFrame->size = packet.size;

    if (packet.stream_index == videoIdx) {
      myFrame->timestamp = (int)round(packet.pts * 90000.0/videoRate);
    }
    if (packet.stream_index == audioIdx) {
      myFrame->timestamp = (int)round(packet.pts * 90000.0/audioRate);
    }

    ret = packet.stream_index;
  }
  else {
    ret = -1;
  }
  av_free_packet(&packet);
  return ret;
}

/* Call this in the end */
void close_context(AVFormatContext *ctx) {
  av_close_input_file(ctx);
}


