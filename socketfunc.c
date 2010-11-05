#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "socketfunc.h"
#include "fileio.h"
#include "util.h"



/* Defined in main.c */
extern int logfd;


int resolve_host(const char *hostname, const char *port, int socktype, 
    int flags, struct addrinfo **info)
{
  struct addrinfo hints;
  int err;

  bzero(&hints, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = socktype;
  hints.ai_flags = flags;

  write_log(logfd, "Resolving %s port %s...\n", hostname, port);

  if ((err = getaddrinfo(hostname, port, &hints, info)) != 0)
  {
    write_log(logfd, "getaddrinfo failed");
    return -1;
  }
  
  return 0;
}



int client_socket(struct addrinfo *info, int bindport)
{
  int sockfd;
  struct addrinfo *step, *ownaddr;
  char portstr[10] = {0};

  for (step = info; step != NULL; step = step->ai_next)
  {
    /* Create the socket */
    if ((sockfd = socket(step->ai_family, step->ai_socktype, step->ai_protocol)) == -1)
    {
      perror("socket()");
      continue;
    }

    if (bindport > 0) {
      sprintf(portstr, "%d", bindport);
      resolve_host(NULL, portstr, SOCK_DGRAM, 0, &ownaddr);
      if (bind(sockfd, ownaddr->ai_addr, ownaddr->ai_addrlen) == -1) {
        fatal_error("bind()");
      } 
      freeaddrinfo(ownaddr);
    }

    /* Connect to the socket */
    if (connect(sockfd, step->ai_addr, step->ai_addrlen) == -1)
    {
      perror("connect()");
      close(sockfd);
      continue;
    }

    /* Connected */
    break;
  }

  if (step == NULL)
  {
    write_log(logfd, "Could not connect to host.\n");
    return -1;
  }

  freeaddrinfo(info);

  return sockfd;
}



int server_socket(struct addrinfo *info)
{
  int sockfd;
  int rebind = 1, val;
  struct addrinfo *step;

  for (step = info; step != NULL; step=step->ai_next)
  {
    if ((sockfd = socket(step->ai_family, step->ai_socktype, step->ai_protocol)) < 0)
    {
      continue;
    }

    /* Prevent TCP from preventing access to the port */
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &rebind, sizeof(int));

    if (bind(sockfd, step->ai_addr, step->ai_addrlen) < 0)
    {
      close(sockfd);
      continue;
    }

    break;
  }

  if (step == NULL)
  {
    write_log(logfd, "Failed to bind.\n");
    if (errno == EADDRINUSE) {
      freeaddrinfo(info);
      write_log(logfd, "Bind error: The port is already in use. Choose another.\n");
      return PORTINUSE;
    }
    fatal_error("bind()");
  }

  freeaddrinfo(info);

  /* Set the socket to non-blocking mode */
  val = fcntl(sockfd, F_GETFL, 0);
  if ((fcntl(sockfd, F_SETFL, val | O_NONBLOCK)) < 0) {
    write_log(logfd, "Failed to set the socket to non-blocking, exiting.");
    exit(EXIT_FAILURE);
  }

  /* Start listening to the socket */
  if (listen(sockfd, 10) == -1)
  {
    fatal_error("listen()");
  }

  return sockfd;

}


void write_remote_ip(char *dest, int sockfd)
{
  struct sockaddr_storage ra;
  socklen_t addrlen = sizeof(ra);

  bzero(dest, INET6_ADDRSTRLEN);

  if (getpeername(sockfd, (struct sockaddr *)&ra, &addrlen) == -1)
  {
    fatal_error("getpeername()");
  }

  if (inet_ntop(ra.ss_family, get_in_addr((struct sockaddr *)&ra),
        dest, INET6_ADDRSTRLEN) == NULL)
  {
    fatal_error("inet_ntop()");
  }

}


void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET)
  {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


unsigned int get_in_port(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET)
  {
    return ((struct sockaddr_in *)sa)->sin_port;
  }

  return ((struct sockaddr_in6 *)sa)->sin6_port;

}

void set_in_port(struct sockaddr *sa, unsigned int port)
{
  if (sa->sa_family == AF_INET) {
    ((struct sockaddr_in *)sa)->sin_port = port;
  }
  else {
    ((struct sockaddr_in6 *)sa)->sin6_port = port;
  }
}


void close_socket(fd_set *master, int sockfd)
{
  close(sockfd);
  FD_CLR(sockfd, master);
}


int Select(int maxfd, fd_set *readfds, struct timeval *timeout)
{
  int nready;

  if ((nready = select(maxfd, readfds, NULL, NULL, timeout)) == -1) {

    if (errno == EINTR) { 
      return nready;
    }
    else {
      fatal_error("select()");
    }
  }

  return nready;
}


int send_all(int sockfd, unsigned char *buf, int size)
{
  int sent = 0, n;
  int left = size;

  printf("Sending %d bytes to socket %d\n", size, sockfd);
  fflush(stdout);
  while (sent < size) {
    n = send(sockfd, buf + sent, left, 0);
    if (n == -1) {
      perror("Send failed:");
      break;
    }
    sent += n;
    left -= n;
  }

  return (n == -1)?-1:sent;
}


int recv_all(int sockfd, unsigned char *buf, size_t len, int flags)
{
  int recvd;
  
  memset(buf, 0, BUFSIZE);

  if ((recvd = recv(sockfd, buf, len, flags)) < 0) {
    fatal_error("recv_all");
  }

  return recvd;
}













