#include "sipfunctions.h"

#include <string.h>
#include <stdio.h>

/* Parses an incoming SIP message from buf and fills the contents                                                                                                    
   to a SIPMsg strucure  */
int parsesipmsg(SIPMsg *msg, const unsigned char *buffer) {
  char *temp = (char *)buffer;
  char *end = strstr((char *)buffer, "\r\n\r\n");
  int typeflag = 0;

  memset(msg, 0, sizeof(SIPMsg));

  while (temp && temp < end) {

    if (!typeflag) {
      if (strncmp(temp, "INVITE", 6) == 0) {
	msg->type = INVITE;
      }
      else if (strncmp(temp, "ACK", 3) == 0) {
	msg->type = ACK;
      }
      else if (strncmp(temp, "BYE", 3) == 0) {
	msg->type = BYE;
      }
      /* Unsupported msg type */
      else return -1;

      msg->fields |= SIP_TYPE;
      typeflag = 1;

    }

    /* Mandatory in all SIP requests are: To, From, Via, CSeq, Call-ID, Max-Forwards */
    if (strncmp(temp, "From: ", 6) == 0) {
      sscanf(temp, "From: %[^\r]\r\n", (char*)&(msg->from));
      msg->fields |= SIP_FROM;
    }
    else if (strncmp(temp, "Via: ", 5) == 0) {
      sscanf(temp, "Via: %[^\r]\r\n", (char*)&(msg->via));
      msg->fields |= SIP_VIA;
    }
    else if (strncmp(temp, "To: ", 3) == 0) {
      sscanf(temp, "To: %[^\r]\r\n", (char*)&(msg->to));
      msg->fields |= SIP_TO;
    }
    else if (strncmp(temp, "CSeq: ", 6) == 0) {
      sscanf(temp, "CSeq: %[^\r]\r\n", (char*)&(msg->cseq));
      msg->fields |= SIP_CSEQ;
    }
    else if (strncmp(temp, "Call-ID: ", 9) == 0) {
      sscanf(temp, "Call-ID: %[^\r]\r\n", (char*)&(msg->callid));
      msg->fields |= SIP_CALLID;
    }
    else if (strncmp(temp, "Content-Length: ", 16) == 0) {
      sscanf(temp, "Content-Length: %d\r\n", &(msg->contentlen));
      msg->fields |= SIP_CONTENTLEN;
    }
    else if (strncmp(temp, "Content-Type: ", 14) == 0) {
      sscanf(temp, "Content-Type: %s\r\n", (char *)&(msg->contenttype));
      msg->fields |= SIP_CONTENTTYPE;
    }

    temp = strstr(temp, "\r\n") + 2;

  }

  if (msg->contentlen > 0 && (strcmp(msg->contenttype, "application/sdp")==0) ) {
    /* Dig out client's RTP port and IP address */
    temp = strstr(temp, "m=audio");
    sscanf(temp, "m=audio %s\r\n", (char *)&(msg->clirtpport));
    msg->fields |= SIP_CLIRTPPORT;
  }

  return 0;
}

/* Creates an appropriate OK for given INVITE */
int create_ok(const SIPMsg *msg, SIPMsg *ok) {
  memset(ok, 0, sizeof(SIPMsg));

  ok->type = SIPOK;
  strcpy(ok->via, msg->via);
  strcpy(ok->to, msg->to);
  strcpy(ok->from, msg->from);  
  strcpy(ok->callid, msg->callid);
  strcpy(ok->cseq, msg->cseq);

  if (msg->type == INVITE) {
    strcpy(ok->contenttype, msg->contenttype);
    sprintf(ok->contents,
	    "v=0\r\n"
	    "o=kaisa 123456 654321 IN IP4 127.0.0.1\r\n"
	    "s=SIP server\r\n"
	    "c=IN IP4 127.0.0.1\r\n"
	    "t=0 0\r\n"
	    "m=audio 0 RTP/AVP 8\r\n"
	    "a=rtpmap:8 PCMA/8000/1\r\n");

    ok->contentlen = strlen(ok->contents);
    ok->fields |= (SIP_CONTENTS | SIP_CONTENTTYPE);
  }
  else if (msg->type == BYE) {
    ok->contentlen = 0;
  }

  ok->fields |= (SIP_TYPE | SIP_VIA | SIP_FROM | SIP_TO | SIP_CONTENTLEN | SIP_CALLID | SIP_CSEQ);

  return 0;
}

/* Writes out the contents of the given SIPMsg to buffer */
/* Handles only 200 OK and BYE */
int write_sip(const SIPMsg *msg, unsigned char *buf, const char *sipport) {
  char *temp = (char *)buf;
  memset(temp, 0, BUFLEN);

  if (msg->type == SIPOK) {
    sprintf(temp, "SIP/2.0 200 OK\r\n");
  }
  else if (msg->type  == BYE) {
    sprintf(temp, "BYE sip:kaisa@localhost SIP/2.0");
  }
  else {
    printf("write_sip: unsupported msg type\n");
    return -1;
  }
  temp += strlen(temp);

  sprintf(temp, "Via: %s\r\n", msg->via);
  temp += strlen(temp);

  sprintf(temp, "From: %s\r\n", msg->from);
  temp += strlen(temp);

  sprintf(temp, "To: %s\r\n", msg->to);
  temp += strlen(temp);

  sprintf(temp, "Call-ID: %s\r\n", msg->callid);
  temp += strlen(temp);

  sprintf(temp, "CSeq: %s\r\n", msg->cseq);
  temp += strlen(temp);

  sprintf(temp, "Contact: %s>\r\n", msg->from);
  temp += strlen(temp);

  sprintf(temp, "Max-Forwards: 70\r\n");
  temp += strlen(temp);

  if (msg->fields & SIP_CONTENTTYPE) {
    sprintf(temp, "Content-Type: %s\r\n", msg->contenttype);
    temp += strlen(temp);
  }

  if (msg->fields & SIP_CONTENTLEN) {
    sprintf(temp, "Content-Length: %d\r\n", msg->contentlen);
    temp += strlen(temp);
  }

  sprintf(temp, "\r\n");
  temp += 2;

  if ( (msg->fields & SIP_CONTENTLEN) && msg->contentlen > 0 ) {
    sprintf(temp, "%s\r\n", msg->contents);
  }

  return 0;
}
