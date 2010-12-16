#include <stdio.h>
#include <glib.h>

#include "parse_video.h"

/* Call this first
   Initializes AVFormatContext
   Returns number of streams in file, -1 if there's an error */
int initialize_context(AVFormatContext **ctx, char *filename, int *videoIdx, int *audioIdx, 
		       double *videoRate, double *audioRate, unsigned char **sps, size_t *spslen,
           unsigned char **pps, size_t *ppslen) {

  int videoStream, audioStream;
  size_t i;
  char *tempstr, *comma, *end;
  char tempbuf[2048];


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

  printf("Finished checking the streams...\n");
  fflush(stdout);

  /* Check frame rate */
  if (videoStream != -1) {
    AVCodecContext *cod = (*ctx)->streams[videoStream]->codec;
    *videoRate = 0.5 * (double)cod->time_base.den / (double)cod->time_base.num;
  }
  else {
    *videoRate = -1;
  }

  if (audioStream != -1) {
    AVCodecContext *cod = (*ctx)->streams[audioStream]->codec;
    *audioRate = (double)cod->sample_rate;
  }

  printf("Finished checking the framerate...\n");
  fflush(stdout);

  /* Create and decode the sprop-parameter-set */
  
  memset(tempbuf, 0, 2048);
  if (avf_sdp_create(ctx, 1, tempbuf, 2048) != 0 ||
      (tempstr = strstr(tempbuf, "sprop-parameter-sets=")) == NULL ||
      (comma = strchr(tempstr, ',')) == NULL ||
      (end = strstr(comma, "\r\n")) == NULL) {
    printf("Error creating the sdp!\n");
    exit(1);
  }

  *comma = '\0';
  *end = '\0';
  
  *sps = g_base64_decode(tempstr + 21, spslen);
  *pps = g_base64_decode(comma + 1, ppslen);
  
  /*
  sprintf(newmsg.data, 
      "v=0\r\n"
      "o=atte\r\n"
      "s=mpeg4video\r\n"
      "t=0 0\r\n"
      "a=recvonly\r\n"
      "m=video 40404 RTP/AVP 96\r\n"
      "a=rtpmap:96 H264/90000\r\n"
      "a=control:trackID=65536\r\n"
      "a=fmtp:96 profile-level-id=42C00D; packetization-mode=1; "
      "sprop-parameter-sets=Z0LADZpzAoP2AiAAAAMAIAAAAwPR4oVN,aM48gA==\r\n"
      "a=framesize:96 320-240\r\n"
      );
  */

  return (*ctx)->nb_streams;
}

/* Call this to get the frames. Allocate yourself memory to myFrame
   Returns videoIdx/audioIdx, indicating to which stream this frame belongs to
   Returns negative number if all frames are read (or error) */
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
      myFrame->timestamp = (int)round(packet.pts * 90000.0/videoRate) + 2;
    }
    if (packet.stream_index == audioIdx) {
      myFrame->timestamp = (int)round((packet.pts / 1024) * (90000.0/50.0)) + 2;
      /*printf("Audio packet timestamp: %ld\n", packet.pts); */
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


