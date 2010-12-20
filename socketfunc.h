#ifndef HTTP2RTSP_SOCKETFUNC_H
#define HTTP2RTSP_SOCKETFUNC_H


#include <netdb.h>

#define PORTINUSE -2
#define GENERALERROR -3
#define BUFSIZE 64000

extern int logfd;

/* A wrapper for getaddrinfo(). In case of error exits the program. */
int resolve_host(const char *hostname, const char *port, int socktype, 
    int flags, struct addrinfo **info);


/* A wrapper for socket(), bind() and listen(). Returns the opened socket descriptor.
 * The created socket will be set into non-blocking mode.
 * Exits the program if it is not able to open or connect to the socket. */
int server_socket(struct addrinfo *info);


/* A wrapper for socket() and connect(). Returns the opened socket descriptor.
   Exits the program if it is not able to open or connect the socket. */
int client_socket(struct addrinfo *info, int bindport);


/* A helper function returning the sockaddr_in on sockaddr_in6 */
void *get_in_addr(struct sockaddr *sa);


/* Gets the port number, independent of IP version */
unsigned int get_in_port(struct sockaddr *sa);

/* Sets the port number, mainly used for binding local sockets */
void set_in_port(struct sockaddr *sa, unsigned int port);

/* Closes the given socket removes the descriptor from the fd_set. */
void close_socket(fd_set *master, int sockfd);


/* Writes the ip address of the remote end of a connected socket to the
 * character array given as an argument. */
void write_remote_ip(char *dest, int sockfd);


/* Wrapper for select, continues on EINTR and exits on other error */
int Select(int maxfd, fd_set *readfds, struct timeval *timeout);

/* Wrapper for send, upon error returns -1 and prints errno */
int send_all(int sockfd, unsigned char *buf, int size);

/* Wrapper for recv */
int recv_all(int sockfd, unsigned char *buf, size_t len, int flags);

/* Wrapper for recvfrom */
int Recvfrom(int sockfd, unsigned char *buffer, int buflen, int flags,
             struct sockaddr *addr, socklen_t *addrsize);

int Sendto_all(int sock, unsigned char *buf, int buflen, int flags,
               struct sockaddr *dest, socklen_t addrlen);

/* Creates a "server" UDP socket */
int udp_server(const char *host, const char *serv, socklen_t *addrlenp);

/* Creates a connected UDP socket */
int udp_connected(const struct sockaddr *cliaddr, socklen_t clilen);

#endif

