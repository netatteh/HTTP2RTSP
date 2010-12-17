#ifndef PARSESIP_H
#define PARSESIP_H

#define FIELDLEN 100
#define CONTENTLEN 3000
#define PORTLEN 10
#define BUFLEN 5000

enum sipmsgtypes {
  INVITE,
  SIPOK,
  ACK,
  BYE
};

enum sipfields {
  SIP_TYPE = 0x1,
  SIP_VIA = 0x2,
  SIP_FROM = 0x4,
  SIP_TO = 0x8,
  SIP_CSEQ = 0x20,
  SIP_CALLID = 0x40,
  SIP_CONTENTLEN = 0x80,
  SIP_CONTENTS = 0x100,
  SIP_CLIRTPPORT = 0x200,
  SIP_CONTENTTYPE = 0x300
};

typedef struct sipmsg {
  int fields;
  int type;
  char to[FIELDLEN];
  char from[FIELDLEN];
  char clirtpport[PORTLEN];
  char via[FIELDLEN];
  char callid[FIELDLEN];
  char cseq[FIELDLEN];
  int contentlen;
  char contenttype[FIELDLEN];
  char contents[CONTENTLEN];
} SIPMsg;

typedef struct cli_info {
  char callid[FIELDLEN];
  int sockfd;
} SIPClient;

/* Parses an incoming SIP message from buf and fills the contents
   to a SIPMsg strucure  */
int parsesipmsg(SIPMsg *msg, const unsigned char *buf);

/* Creates an appropriate OK for given INVITE or BYE */
int create_ok(const SIPMsg *msg, SIPMsg *ok);

/* Writes out the contents of the given SIPMsg to buffer */
int write_sip(const SIPMsg *msg, unsigned char *buf);

#endif
