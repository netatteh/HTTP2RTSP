#ifndef PARSE_VIDEO_H
#define PARSE_VIDEO_H

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#define VIDEO_FRAME 0
#define AUDIO_FRAME 1


/* Definition of frame struct */
typedef struct frame {
  uint8_t *data;
  size_t size;
  uint32_t timestamp;
  int frametype;
} Frame;

/* Call this function to oprn the file and initialize AVFormatContext. 
 * Creates the SDP of the file and stores it into the 'sdpbuf'
 * Returns: number of streams in the filee (1 if only video, 2 if video&audio,
 * -1 if there's an error. */
int initialize_context(AVFormatContext **ctx, char *filename, int *videoIdx, int *audioIdx,
		       double *videoRate, double *audioRate, unsigned char **sps, size_t *spslen,
           unsigned char **pps, size_t *ppslen);

/* Call this in a loop to get the frames
   Allocate memory for myFrame yourself, the function will then fill it for you
   Returns: videoIdx if it's a video frame, audioIdx if it's an audio frame, -1 when all frames are read */
int get_frame(AVFormatContext *ctx, Frame *myFrame, int videoIdx, int audioIdx,
	      double videoRate, double audioRate);

/* Close file and context */
void close_context(AVFormatContext *ctx);

#endif
