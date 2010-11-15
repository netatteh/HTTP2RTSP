#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdarg.h>

#include "fileio.h"
#include "util.h"

pthread_mutex_t filemut = PTHREAD_MUTEX_INITIALIZER;


/* Inspiration from UAP lecture slides */
int writestr(int fd, const void *str, size_t length)
{
    size_t nleft;
    ssize_t nwritten; 
    unsigned char *ptr;

    /* Write to the file */
    ptr = (unsigned char *)str;
    nleft = length;
    while (nleft > 0)
    {
        if ((nwritten = write(fd, ptr, nleft)) <= 0)
        {
            if (errno == EINTR)
            {
                nwritten = 0;
            } 
            else
            {
                return -1;
            }
        }
        nleft -= nwritten;
        /* Need to remember where the write was interrupted */
        ptr += nwritten;
    }   

    return length;
} 


void write_log(int fd, char *format, ...)
{
    va_list ap;
    char *temp;
    int len;
    time_t curtime;
    char timestr[64];
    char logbuf[LOGBUFSIZE];
    size_t timelength;

    lock_mutex(&filemut);

    memset(logbuf, 0, LOGBUFSIZE * sizeof(char));
    memset(timestr, 0, 64 * sizeof(char));

    time(&curtime);
    ctime_r(&curtime, timestr);
    timelength = strlen(timestr);
    temp = strchr(timestr, '\n');
    *temp = ']';

    logbuf[0] = '[';
    strcpy(logbuf + 1, timestr);
    logbuf[timelength + 1] = ' ';

    sprintf(logbuf + timelength + 2, "%d - ", (int)getpid());
    len = strlen(logbuf + timelength + 2);
    va_start(ap, format);
    
    vsprintf(logbuf + timelength + 2 + len, format, ap);
    va_end(ap);

    if (writestr(fd, logbuf, strlen(logbuf)) == -1)
    {
        printf("Error printing to log\n");
    }

    unlock_mutex(&filemut);

}


/* Lock the given mutex */
void lock_mutex(pthread_mutex_t *mut)
{
    /* printf("Process %d locking mutex\n", getpid()); */
    if (pthread_mutex_lock(mut) != 0)
    {
        fatal_error("Locking mutex failed");    
    }
}



/* Unlocks the given mutex */
void unlock_mutex(pthread_mutex_t *mut)
{
    /* printf("Process %d unlocking mutex\n", getpid()); */
    if (pthread_mutex_unlock(mut) != 0)
    {
        fatal_error("Unlocking mutex failed");    
    }
}



