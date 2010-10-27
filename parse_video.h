#ifndef PARSE_VIDEO_H
#define PARSE_VIDEO_H

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

// Definition of frame struct
struct frame {
  uint8_t *data;
  int size;
  uint32_t timestamp;
};

// Call this function to oprn the file and initialize AVFormatContext
// Returns: number of streams in the filee (1 if only video, 2 if video&audio,
// -1 if there's an error
int initialize_context(AVFormatContext **ctx, char *filename, int *videoIdx, int *audioIdx,
		       double *videoRate, double *audioRate);

// Call this in a loop to get the frames
// Allocate memory for myFrame yourself, the function will then fill it for you
// Returns: videoIdx if it's a video frame, audioIdx if it's an audio frame, -1 when all frames are read
int get_frame(AVFormatContext *ctx, struct frame *myFrame, int videoIdx, int audioIdx,
	      double videoRate, double audioRate);

// Close file and context
void close_context(AVFormatContext *ctx);

#endif
