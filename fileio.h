#ifndef HTTP2RTSP_FILEIO_H
#define HTTP2RTSP_FILEIO_H

#define LOGBUFSIZE 256

/* Writes the contents of str to fd */
int writestr(int fd, const void *str, size_t length);


/* Writes the contents of str to fd, adding a timestamp */
void write_log(int fd, char *format, ...);


/* Locks the mutex given as an argument. */
void lock_mutex(pthread_mutex_t *mut);


/* Unlocks the mutec given as an argument. */
void unlock_mutex(pthread_mutex_t *mut);

#endif /* HTTP2RTSP_FILEIO_H */

