#ifndef MT_SERVER_H
#define MT_SERVER_H

#define MAXCLIENTS 10

/* Like start_server in server.c but uses SIP */
int start_mt_server(const char *url, const char *rtspport, const char *sipport);

#endif

