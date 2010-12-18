#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <search.h>
#include <glib.h>

#include "httpmsg.h"
#include "socketfunc.h"
#include "fileio.h"
#include "util.h"
#include "parse_video.h"
#include "send_frame.h"

#include "mt_server.h"
#include "sipfunctions.h"
#include "server.h"

extern int logfd;
extern pthread_mutex_t queuelock;
extern pthread_cond_t queuecond;
extern Queue queue;

int start_mt_server(const char *url, const char *rtspport, const char *sipport)
{
  int mediafd = -1, listenfd, tempfd, maxfd;
  int videofd;
  struct addrinfo *info;
  struct sockaddr_storage remoteaddr;
  socklen_t addrlen = sizeof remoteaddr;
  fd_set readfds, masterfds;
  struct timeval *timeout, *timeind = NULL, timenow;
  int nready, i;
  int videosize, videoleft;
  int recvd, sent;
  char urlhost[URLSIZE], urlpath[URLSIZE], tempstr[URLSIZE];
  unsigned char msgbuf[BUFSIZE], sendbuf[BUFSIZE];
  char *temp;
  unsigned char *sps = NULL, *pps = NULL;
  size_t spslen, ppslen;
  RTSPMsg rtspmsg;
  Client streamclient;
  pthread_t threadid;
  ThreadInfo *tinfo = NULL;

  uint16_t rtpseqno_video = (rand() % 1000000);
  uint16_t rtpseqno_audio = rtpseqno_video;
  TimeoutEvent *event;

  /* The current state of the protocol */
  int mediastate = IDLE;
  int quit = 0;

  /* For SIP */
  int siplistenfd, rtpfd, nclients=0, k;
  SIPMsg sipmsg, ok;
  SIPClient *client;
  SIPClient *clientlist[MAXCLIENTS];
  struct sockaddr cliaddr, rtpaddr;
  socklen_t clilen;
  unsigned char sip_inbuf[BUFSIZE];
  unsigned char sip_outbuf[BUFSIZE];
  fd_set sipmasterfds, sipwritefds;

  FD_ZERO(&sipmasterfds);
  FD_ZERO(&sipwritefds);

  /* Initialize SIP client list */
  for (k=0; k<MAXCLIENTS; k++) {
    clientlist[k] = NULL;
  }

  /* Create UDP socket and bind to given port */
  siplistenfd = udp_server(NULL, sipport, &clilen);

  timeout = (struct timeval *)malloc(sizeof(struct timeval));
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
  FD_SET(siplistenfd, &masterfds);

  while (!quit) {

    readfds = masterfds;

    if ((nready = Select(maxfd + 1, &readfds, timeind)) == -1) {
      write_log(logfd, "Select interrupted by a signal\n");
    } 
    /* Timeout handling, used for packet pacing and other timeouts */
    else if (nready == 0) {
      timeind = NULL;
      lock_mutex(&queuelock);
      if ((event = pull_event(&queue)) != NULL) {

        switch (event->type) {

	case FRAME:

	  /* Video frame */
	  if (event->frame->frametype == VIDEO_FRAME) {
	    rtpseqno_video += send_video_frame(sendbuf, event->frame, streamclient.videofds[0], rtpseqno_video);
	  }

	  /* Audio frame */
	  else {
	    /* Send the audio frame to all SIP clients who have sent ACK */
	    for (k=0; k<MAXCLIENTS; k++) {
	      if (clientlist[k] != NULL && clientlist[k]->ackrecvd == 1) {
		rtpseqno_audio += send_audio_frame(sendbuf, event->frame, clientlist[k]->sockfd, rtpseqno_audio);
	      }
	    }
	    
            /* rtpseqno_audio += send_audio_frame(sendbuf, event->frame, streamclient.audiofds[0], rtpseqno_audio); */
          }

          free(event->frame->data);
          free(event->frame);
          break;

	case CHECKMEDIASTATE:
	  oma_debug_print("Timeout handling checking media state...\n");
	  if (mediastate != STREAM) {
	    send_dummy_rtp(sendbuf, streamclient.videofds[0], &rtpseqno_video);
	    push_timeout(&queue, 1000, CHECKMEDIASTATE);
	  }
          break;

	case ENDOFSTREAM:
	  /* TODO: send TEARDOWN to VLC */

	  /* Send bye to SIP clients*/
	  bzero(sip_outbuf, BUFSIZE);
	  create_bye(&sipmsg, clientlist[k]);
	  write_sip(&sipmsg, sip_outbuf, sipport);

	  printf("Sending BYE to SIP clients...\n");

	  for (k=0; k<MAXCLIENTS; k++) {
	    if (clientlist[k] != NULL && clientlist[k]->ackrecvd == 1) {
	      Sendto_all(siplistenfd, sip_outbuf, strlen((char*)sip_outbuf), 0, &(clientlist[k]->cliaddr), sizeof(clientlist[k]->cliaddr));
	    }
	  }
	  break;

	default:
	  oma_debug_print("ERRORENOUS EVENT TYPE!\n");
          break;
        }
	/* If there are elements left in the queue, calculate next timeout */
        if (queue.size > 0) {
          *timeout = calculate_delta(&event->time, &queue.first->time);
          timeind = timeout;
          oma_debug_print("Timeout: %ld secs, %ld usecs\n", timeout->tv_sec, timeout->tv_usec);
        }
        else {
          oma_debug_print("The first entry of the queue is NULL!\n");
        }

        if (queue.size < QUEUESIZE / 2) {
          oma_debug_print("Signaling thread to start filling the queue");
          pthread_cond_signal(&queuecond);
        }

        free(event);
      }

      unlock_mutex(&queuelock);
      continue;

    } /* End of timeout handling */

    /* Start to loop through the file descriptors */
    for (i = 0; i <= maxfd; i++) {

      if (FD_ISSET(i, &readfds)) {

        nready--;

        /* New connection from a client */
        if (i == listenfd) {
	  oma_debug_print("Recieved a new connection to listenfd\n");
          if ((tempfd = accept(i, (struct sockaddr *)&remoteaddr, &addrlen)) == -1) {
            if (errno != EWOULDBLOCK && errno != ECONNABORTED &&
                errno != EPROTO && errno != EINTR) {
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

	/* New SIP client */
	else if (i == siplistenfd) {
	  bzero(sip_inbuf, BUFLEN);
	  bzero(&cliaddr, clilen);
	  Recvfrom(siplistenfd, sip_inbuf, BUFSIZE, 0, &cliaddr, &clilen);

	  bzero(&sipmsg, sizeof(&sipmsg));
	  parsesipmsg(&sipmsg, sip_inbuf);

	  /* Received INVITE */
	  if (sipmsg.type == INVITE) {
	    printf("SIP INVITE received\n");

	    if (nclients < MAXCLIENTS) {
	      bzero(&ok, sizeof(&ok));
	      bzero(sip_outbuf, sizeof(sip_outbuf));
	      
	      create_ok(&sipmsg, &ok);
	      write_sip(&ok, sip_outbuf, sipport);
	      Sendto_all(siplistenfd, sip_outbuf, strlen((char*)sip_outbuf), 0, &cliaddr, clilen);

	      printf("SIP 200 OK sent\n");

	      rtpaddr = cliaddr;

	      /* Create connected UDP socket to client's rtp port */
	      if (rtpaddr.sa_family == AF_INET) {
		((struct sockaddr_in*)&rtpaddr)->sin_port = htons(atoi(sipmsg.clirtpport));
	      }
	      else if (rtpaddr.sa_family == AF_INET6) {
		((struct sockaddr_in6*)&rtpaddr)->sin6_port = htons(atoi(sipmsg.clirtpport));
	      }
	      rtpfd = udp_connected(&rtpaddr, clilen);

	      /* Create new client */
	      client = malloc(sizeof(SIPClient));
	      strcpy((char*)client->callid, (char*)sipmsg.callid);
	      client->sockfd = rtpfd;
	      client->ackrecvd = 0;

	      /* Save client info to first free position in clientlist */
	      for (k=0; k<MAXCLIENTS; k++) {
		if (clientlist[k] == NULL) {
		  clientlist[k] = client;
		  FD_SET(clientlist[k]->sockfd, &sipmasterfds);
		  nclients++;
		  write_log(logfd, "SIP client accepted\n");
		  break;
		}
	      }

	    }
	    else {
	      write_log(logfd, "SIP client rejected: MAXCLIENTS reached\n");
	      printf("SIP MAXCLIENTS reached! Cannot accept new SIP clients\n");
	    }
	  }
	  else if (sipmsg.type == ACK) {
	    printf("SIP ACK received\n");

	    /* Add client to writefds  */
	    for (k=0; k<MAXCLIENTS; k++) {
	      if ( (clientlist[k] != NULL) && (strcmp(clientlist[k]->callid, sipmsg.callid)==0) ) {
		clientlist[k]->ackrecvd = 1;
		strcpy(clientlist[k]->from, sipmsg.from);
		strcpy(clientlist[k]->to, sipmsg.to);
		clientlist[k]->cliaddr = cliaddr;
		break;
	      }
	    }
	  }
	  else if (sipmsg.type == BYE) {
	    printf("SIP BYE received\n");
	    
	    /* Send OK message */
	    create_ok(&sipmsg, &ok);
	    write_sip(&ok, sip_outbuf, sipport);
	    Sendto_all(siplistenfd, sip_outbuf, BUFLEN, 0, &cliaddr, clilen);
	    printf("SIP 200 OK sent\n");
	    sleep(5);

	    /* Find and remove client info */
	    for (k=0; k<MAXCLIENTS; k++) {
	      if ( (clientlist[k] != NULL) && (strcmp(clientlist[k]->callid, sipmsg.callid)==0) ) {
		/* Remove client info */
		FD_CLR(clientlist[k]->sockfd, &sipmasterfds);
		close(clientlist[k]->sockfd);
		free(clientlist[k]);
		clientlist[k] = NULL;
		nclients--;
		break;
	      }
	    }
	  }
	  else {
	    /* Unknown message type received */
	    printf("SIPServer received unsupported msg type\n");
	    return -1;
	  }
	} /* Finished handling SIP msg */

        /* Data from the media source */
        else if (i == mediafd) {

	  switch(mediastate) {

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
	      oma_debug_print("Socket closed\n");
	    }
	    oma_debug_print("Received data from video source!\n");

	    writestr(videofd, msgbuf, recvd);
	    videoleft -= recvd;

	    if (videoleft <= 0) {
	      oma_debug_print("Video download complete!\n");
	      FD_CLR(mediafd, &masterfds);
	      close(videofd);
	      close(mediafd);

	      /* Create the context and the queue filler thread parameter struct */
	      tinfo = (ThreadInfo *)malloc(sizeof(ThreadInfo));
	      initialize_context(&tinfo->ctx, "videotemp.mp4", &tinfo->videoIdx, &tinfo->audioIdx,
				 &tinfo->videoRate, &tinfo->audioRate, &sps, &spslen, &pps, &ppslen);
	      /* Launch the queue filler thread */
	      CHECK((pthread_create(&threadid, NULL, fill_queue, tinfo)) == 0);
	      pthread_detach(threadid);

	      /* Send the sprop-parameters before any other frames */
	      send_video_frame(sendbuf, create_sprop_frame(sps, spslen, 0),
			       streamclient.videofds[0], rtpseqno_video++);
	      send_video_frame(sendbuf, create_sprop_frame(pps, ppslen, 0),
			       streamclient.videofds[0], rtpseqno_video++);

	      g_free(sps);
	      g_free(pps);


	      lock_mutex(&queuelock);
	      push_timeout(&queue, 1000, CHECKMEDIASTATE);
	      unlock_mutex(&queuelock);


	      mediastate = STREAM;
	    }
	    break;

	  case STREAM:
	  
	    break;

	  default:
	    break;
	  }

	}
	
	/* Data from a client ( i == streamclient.rtspfd) */
	else {

	  oma_debug_print("Received data from rtspfd\n");

	  if ((recvd = recv_all(i, msgbuf, BUFSIZE, 0)) == 0) {
	    FD_CLR(i, &masterfds);
	    close(i);
	    oma_debug_print("Socket closed\n");
	    streamclient.state = NOCLIENT;
	  }
	  else {
	    oma_debug_print("%s", msgbuf);
	    parse_rtsp(&rtspmsg, msgbuf);
	  }

	  switch (streamclient.state) {
	  case CLICONNECTED:
	    if (rtspmsg.type == OPTIONS) {
	      sent = rtsp_options(&rtspmsg, &streamclient, sendbuf);
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

	      /* Send the SDP without sprop-parameter-sets, those are sent                                                                                          
	       * later in-band */
	      streamclient.state = SDPSENT;
	      sent = rtsp_describe(&streamclient, sendbuf);
	      send_all(i, sendbuf, sent);
	    }
	    break;

	  case SDPSENT:
	    if (rtspmsg.type == SETUP) {
	      streamclient.setupsreceived++;

	      /* Open up the needed ports and bind them locally. The RTCP ports opened here                                                                         
	       * are not really used by this application. */
	      write_remote_ip(tempstr, streamclient.rtspfd);
	      oma_debug_print("Remote IP: %s\n", tempstr);

	      if (streamclient.setupsreceived < 2) {
		resolve_host(tempstr, rtspmsg.clirtpport, SOCK_DGRAM, 0, &info);
		streamclient.audiofds[0] = client_socket(info, streamclient.server_rtp_audio_port);
		resolve_host(tempstr, rtspmsg.clirtcpport, SOCK_DGRAM, 0, &info);
		streamclient.audiofds[1] = client_socket(info, streamclient.server_rtcp_audio_port);

		sent = rtsp_setup(&rtspmsg, &streamclient, sendbuf,
				  streamclient.server_rtp_audio_port, streamclient.server_rtcp_audio_port);

	      }
	      else {
		resolve_host(tempstr, rtspmsg.clirtpport, SOCK_DGRAM, 0, &info);
		streamclient.videofds[0] = client_socket(info, streamclient.server_rtp_video_port);
		resolve_host(tempstr, rtspmsg.clirtcpport, SOCK_DGRAM, 0, &info);
		streamclient.audiofds[1] = client_socket(info, streamclient.server_rtcp_video_port);
		
		sent = rtsp_setup(&rtspmsg, &streamclient, sendbuf,
				  streamclient.server_rtp_video_port, streamclient.server_rtcp_video_port);

		streamclient.state = SETUPCOMPLETE;
	      }

	      oma_debug_print("Sending setup response...\n");
	      send_all(i, sendbuf, sent);

	    }
	    break;

	  case SETUPCOMPLETE:
	    if (rtspmsg.type == PLAY) {

	      /* Respond to the PLAY request, and start sending dummy RTP packets                                                                                   
	       * to disable the client timeout */
	      sent = rtsp_play(&rtspmsg, sendbuf);
	      send_all(i, sendbuf, sent);
	      lock_mutex(&queuelock);
	      push_timeout(&queue, 100, CHECKMEDIASTATE);
	      unlock_mutex(&queuelock);
	    }

	    break;

	  default:
	  break;
	}
      }
    }

    if (nready <= 0) break;   
  }

  /* Set the timeout value again, since select will mess it up */
  lock_mutex(&queuelock);
  if (queue.size > 0) {
    CHECK((gettimeofday(&timenow, NULL)) == 0);
    *timeout = calculate_delta(&timenow, &queue.first->time);
    oma_debug_print("Delta sec: %ld, Delta usec: %ld\n", timeout->tv_sec, timeout->tv_usec);
    
    if (timeout->tv_sec < 0) {
      timeout->tv_sec = 0;
      timeout->tv_usec = 0;
    }

    timeind = timeout;
  }
  else timeind = NULL;
  unlock_mutex(&queuelock);


}


  return 1;
}


