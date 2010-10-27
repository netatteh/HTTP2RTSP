#ifndef HTTP2RTSP_UTIL_H
#define HTTP2RTSP_UTIL_H


/* A helper macro for checking expressions, taken from S-38.3600 slides */
#define CHECK(EXPR) do {\
     if (!((EXPR) + 0)) {\
            perror(#EXPR);\
            fprintf(stderr,"CHECK: Error on process %d file %s line %d\n",\
                       getpid(), __FILE__,__LINE__);\
            fputs("CHECK: Failing assertion: " #EXPR "\n", stderr);\
            exit(1);\
          }\
} while (0)


/* Prints error message and exits */
void fatal_error(const char *msg);


/* min and max funtions taking variable arguments. The 'count' argument 
 * tells how many numbers need to be compared. */

int max(int count, ...);

int min(int count, ...);


#endif

