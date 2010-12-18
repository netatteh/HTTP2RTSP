#include <stdio.h>
#include <glib.h>


#include "fileio.h"
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

  oma_debug_print("Finished checking the streams...\n");
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

  oma_debug_print("Finished checking the framerate...\n");
  fflush(stdout);

  /* Create and decode the sprop-parameter-set */
  
  memset(tempbuf, 0, 2048);
  if (avf_sdp_create(ctx, 1, tempbuf, 2048) != 0 ||
      (tempstr = strstr(tempbuf, "sprop-parameter-sets=")) == NULL ||
      (comma = strchr(tempstr, ',')) == NULL ||
      (end = strstr(comma, "\r\n")) == NULL) {
    oma_debug_print("Error creating the sdp!\n");
    exit(1);
  }

  *comma = '\0';
  *end = '\0';
  
  *sps = g_base64_decode(tempstr + 21, spslen);
  *pps = g_base64_decode(comma + 1, ppslen);
  
  return (*ctx)->nb_streams;
}

/* Call this to get the frames. Allocate yourself memory to myFrame
   Returns videoIdx/audioIdx, indicating to which stream this frame belongs to
   Returns negative number if all frames are read (or error) */
int get_frame(AVFormatContext *ctx, struct frame *myFrame, int videoIdx, int audioIdx, 
	      double videoRate, double audioRate) {

  AVPacket packet;
  AVCodec *deCodec, *enCodec;
  AVCodecContext *cod, *enCod;
  int ret, frame_size;
  uint8_t *mediabuf;
  uint8_t audiooutbuf[FF_MIN_BUFFER_SIZE];
  int len;

  /* Joku taikatemppu strait outta google, segfaulttaa muuten */
  DECLARE_ALIGNED(16,uint8_t,audioinbuf)[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];
  int out_size = sizeof(audioinbuf); 


  oma_debug_print("Entered get_frame\n");

  if (av_read_frame(ctx, &packet) >= 0) {

    /* Video frame */
    if (packet.stream_index == videoIdx) {
      mediabuf = (uint8_t *)malloc(packet.size*sizeof(uint8_t));
      memcpy(mediabuf, packet.data, packet.size);

      myFrame->data = mediabuf;
      myFrame->size = packet.size;
      myFrame->timestamp = (int)round(packet.pts * av_q2d(ctx->streams[videoIdx]->time_base) * 90000.0);
      oma_debug_print("Video pts: %d\n", packet.pts);
    }

    /* Audio frame */
    else if (packet.stream_index == audioIdx) {
      oma_debug_print("Found audio frame...\n");

      cod = ctx->streams[audioIdx]->codec;
      deCodec = avcodec_find_decoder(cod->codec_id);
      packet.data[packet.size] = 0;

      oma_debug_print("Found decoder...\n");

      if (!deCodec) {
        fprintf(stderr, "Error while decoding audio: Codec not supported!\n");
        return -1;
      }

      avcodec_open(cod, deCodec);

      oma_debug_print("Opened codec for decoding...\n");

      memset(audioinbuf, 0, AVCODEC_MAX_AUDIO_FRAME_SIZE);
      len = avcodec_decode_audio3(cod, (int16_t *)audioinbuf, &out_size, &packet);
      oma_debug_print("AAC frame decoded to %d samples, out_size=%d\n", len, out_size);

      enCodec = avcodec_find_encoder(CODEC_ID_PCM_ALAW);
      if (!enCodec) {
        fprintf(stderr, "Error while encoding audio: Codec not supported!\n");
        return -1;
      }

      enCod = avcodec_alloc_context();
      enCod->bit_rate = cod->bit_rate;
      enCod->sample_rate = cod->sample_rate;
      enCod->channels = cod->channels;
      enCod->sample_fmt = SAMPLE_FMT_S16;

      if (avcodec_open(enCod, enCodec) < 0) {
        fprintf(stderr, "Error while opening codec!\n");
        return -1;
      }

      frame_size = enCod->frame_size;
      oma_debug_print("Audio frame size: %d\n", frame_size);
      if ((len = avcodec_encode_audio(enCod, audiooutbuf, len * 6, (int16_t *)audioinbuf)) <= 0) {
        fprintf(stderr, "Error encoding audio: frame\n");
      }
      oma_debug_print("Bytes used after encoding to PCMA: %d, FF_MIN_BUFFER_SIZE=%d\n", len, FF_MIN_BUFFER_SIZE);

      mediabuf = (uint8_t *)malloc(len * sizeof(uint8_t));
      memcpy(mediabuf, audiooutbuf, len);

      myFrame->data = mediabuf;
      myFrame->size = len;
      myFrame->timestamp = (int)round(packet.pts * av_q2d(ctx->streams[audioIdx]->time_base) * 90000.0);
      oma_debug_print("Audio pts: %d\n", packet.pts);

      avcodec_close(enCod);
      av_free(enCod);
    }

    ret = packet.stream_index;
  }
  else {
    oma_debug_print("Error reading frame!!\n");
    ret = -1;
  }

  av_free_packet(&packet);
  return ret;
}

/* Call this in the end */
void close_context(AVFormatContext *ctx) {
  av_close_input_file(ctx);
}


