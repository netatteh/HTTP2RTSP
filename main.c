#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>

#include "server.h"
#include "fileio.h"
#include "util.h"
#include "httpmsg.h"
#include "mt_server.h"

/* File descriptor used for logging, global */
int logfd;

/* Debug flag */
int debug;

/* Signals come here */
sig_atomic_t signo;

void sig_handler(int signum)
{
  signo = signum; 
}


void usage(const char *progname)
{
  printf(
      "Usage: %s -f <http address of source> -l <rtsp listening ports> [-s <sip listening port>]\n",
      progname);
  printf("If parameter -s is given, the Movie theatre scenario (Assignment 2) is started.\n");
  exit(EXIT_SUCCESS);
}


void handle_signal(int signo) {

  struct sigaction act;

  bzero(&act, sizeof(act));
  sigemptyset(&act.sa_mask);
  act.sa_handler = sig_handler;

  if (sigaction(signo, &act, NULL) == -1) {
    fatal_error("sigaction()");
  }
}



int main(int argc, char *argv[])
{
  extern char* optarg;
  extern int optind;
  int c;
  char *logfilename = "debug.log";
  char httpsource[URLSIZE], rtspport[10], sipport[10];


  /* TODO: t is not yet used */
  int f = 0, l = 0, s = 0;

  debug = 0;

  memset(httpsource, 0, URLSIZE);
  memset(rtspport, 0, 10);

  /* Process the command line arguments */
  while ((c = getopt(argc, argv, "f:l:s:")) != -1) {
    switch (c) {
      case 'f':
        strncpy(httpsource, optarg, URLSIZE - 1);
        f = 1;
        break;
      case 'l':
        strncpy(rtspport, optarg, 9);
        l = 1;
        break;
    case 's':
      strncpy(sipport, optarg, 9);
      s = 1;
      break;
      default:
        usage(argv[0]);
    }
  }

  /* Check that all required parameters have been given */
  if (!f || !l) 
    usage(argv[0]);

  /* Do some necessary signal handling. PIPE for double sends,
   * INT and term for cleaning when terminated. */
  /* handle_signal(SIGPIPE); */
  /* handle_signal(SIGINT); */
  /* handle_signal(SIGTERM); */

  
  /* Open the logfile */
  if ((logfd = open(logfilename, O_WRONLY | O_CREAT | O_APPEND, S_IRWXU)) < 0) {
    perror("Error opening the logfile");
    exit(EXIT_FAILURE);
  }

  /* Just a demonstration of how to use the logging functionality.
   * Timestamps are added to the log automatically.
   *write_log(logfd, "Magee logi %d %d %s\n", 3, 4, "Heppa"); */



  /* If -s is given, starts Movie Theatre (assignment 2)*/
  if (s) {
    printf("Starting HTTP2MT proxy server version 0.1 by Helenius, Korhonen & Saarnia\n\n");
    start_mt_server(httpsource, rtspport, sipport);
  }
  /* Otherwise, start HTTP2RTSP (assignment 1) */
  else {
    printf("Starting HTTP2RTSP proxy server version 0.2 by Helenius, Korhonen & Saarnia\n\n");
    start_server(httpsource, rtspport);
  }
  close(logfd);
  

  return 1;
}






















