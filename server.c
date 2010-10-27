#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>

#include "server.h"
#include "httpmsg.h"
#include "socketfunc.h"
#include "fileio.h"
#include "util.h"
#include "parse_video.h"


/* Defined in main.c, used for logging */
extern int logfd;


void init_client(Client *client)
{
  client->state = NOCLIENT;
  client->rtspfd = -1;
  client->videofds[0] = -1;
  client->videofds[1] = -1;
  client->audiofds[0] = -1;
  client->audiofds[1] = -1;
}



int start_server(const char *url, const char *rtspport)
{
  int mediafd = -1, listenfd, tempfd, maxfd;
  int videofd;
  struct addrinfo *info;
  struct sockaddr_storage remoteaddr;
  socklen_t addrlen = sizeof remoteaddr;
  fd_set readfds, masterfds;
  int nready, i;
  int videosize, videoleft;
  int recvd, sent;
  char urlhost[URLSIZE], urlpath[URLSIZE], tempstr[URLSIZE];
  unsigned char msgbuf[BUFSIZE], sendbuf[BUFSIZE];
  char *temp;
  RTSPMsg rtspmsg;
  Client streamclient;


  /* The current state of the protocol */
  int mediastate = IDLE;
  int quit = 0;

  init_client(&streamclient);

  /* Open the a file where the video is to be stored */
  if ((videofd = open("videotemp.mp4", O_RDWR | O_CREAT | O_TRUNC, S_IRWXU)) < 0) {
    fatal_error("Error opening the temporary videofile");
  }

  /* Create the RTSP listening socket */
  resolve_host(NULL, rtspport, SOCK_STREAM, AI_PASSIVE, &info);
  listenfd = server_socket(info);
  maxfd = listenfd;


  FD_ZERO(&readfds);
  FD_ZERO(&masterfds);
  FD_SET(listenfd, &masterfds);


  while (!quit) {

    readfds = masterfds;

    if ((nready = Select(maxfd + 1, &readfds, NULL)) == -1) {
      write_log(logfd, "Select interrupted by a signal\n");
    } 

    for (i = 0; i <= maxfd; i++) {
      if (FD_ISSET(i, &readfds)) {

        nready--;

        /* New connection from a client */
        if (i == listenfd) {
          if ((tempfd = accept(i, (struct sockaddr *)&remoteaddr, &addrlen)) == -1) {
            if (errno != EWOULDBLOCK && errno != ECONNABORTED &&
                errno != EPROTO && errno != EINTR)
            {
              fatal_error("accept");
            }
          }

          /* If we are already serving a client, close the new connection. Otherwise, continue. */
          if (streamclient.state != NOCLIENT) close (tempfd);
          else {
            streamclient.rtspfd = tempfd;
            streamclient.state = CLICONNECTED;
            maxfd = max(2, streamclient.rtspfd, maxfd);
            FD_SET(streamclient.rtspfd, &masterfds);
          }
        }

        /* Data from the media source */
        else if (i == mediafd) {

          switch (mediastate) {

            case GETSENT:
              /* Read ONLY the HTTP message from the socket and store the video size */
              recvd = recv_all(i, msgbuf, BUFSIZE, MSG_PEEK);
              temp = strstr((char *)msgbuf, "\r\n\r\n");
              recvd = recv_all(i, msgbuf, (int)(temp + 4 - (char *)msgbuf), 0);
              temp = strstr((char *)msgbuf, "Content-Length:");
              sscanf(temp, "Content-Length: %d", &videosize);
              videoleft = videosize;
              mediastate = RECVTCP;
              break;

            case RECVTCP:
              if ((recvd = recv_all(i, msgbuf, BUFSIZE, 0)) == 0) {
                FD_CLR(i, &masterfds);
                close(i);
                printf("Socket closed\n");
              }
              writestr(videofd, msgbuf, recvd);
              videoleft -= recvd;
              if (videoleft <= 0) mediastate = STREAM;
              break;

              /* TODO: Start streaming, currently just exits the program */
            case STREAM:
              /*
                 close(videofd);
                 close(mediafd);
                 close(listenfd);
                 quit = 1;
                 */
              break;

            default: 
              break;
          }
        }

        /* Data from a client ( i == streamclient.rtspfd) */
        else {

          if ((recvd = recv_all(i, msgbuf, BUFSIZE, 0)) == 0) {
            FD_CLR(i, &masterfds);
            close(i);
            printf("Socket closed\n");
            streamclient.state = NOCLIENT;
          }
          else {
            printf("%s", msgbuf);
            parse_rtsp(&rtspmsg, msgbuf); 
          }

          switch (streamclient.state) {

            case CLICONNECTED:
              if (rtspmsg.type == OPTIONS) {
                sent = rtsp_options(&rtspmsg, sendbuf);
                send_all(i, sendbuf, sent);
              }
              else if (rtspmsg.type == DESCRIBE) {

                /* Start fetching the file from the server */
                parse_url(url, urlhost, urlpath);
                resolve_host(urlhost, "80", SOCK_STREAM, 0, &info);
                mediafd = client_socket(info, 0);
                FD_SET(mediafd, &masterfds);
                maxfd = max(2, maxfd, mediafd);

                /* Send the GET message */
                http_get(url, msgbuf);
                send_all(mediafd, msgbuf, strlen((char *)msgbuf));
                mediastate = GETSENT;

                /* TODO: parse SDP from the media file rather than hardcoding it */
                sent = rtsp_describe(&rtspmsg, sendbuf);
                send_all(i, sendbuf, sent);
                streamclient.state = SDPSENT;
              }
              break;

            case SDPSENT:
              if (rtspmsg.type == SETUP) {
                sent = rtsp_setup(&rtspmsg, sendbuf, 50508, 50509);
                send_all(i, sendbuf, sent);
                write_remote_ip(tempstr, streamclient.rtspfd);
                resolve_host(tempstr, rtspmsg.clirtpport, 0, SOCK_DGRAM, &info); 
                streamclient.videofds[0] = client_socket(info, 50508);
                resolve_host(tempstr, rtspmsg.clirtcpport, 0, SOCK_DGRAM, &info);
                streamclient.videofds[1] = client_socket(info, 50509);
                streamclient.state = SETUPSENT;
              }
              break;

            case SETUPSENT:
              if (rtspmsg.type == PLAY) {
              }
              
              break;

            default:
              break;
          }
        }
      }
      if (nready <= 0) break;   
    }

  }


  return 1;
}




