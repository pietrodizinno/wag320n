/* $Id: upnphttp.c,v 1.2 2009-07-24 05:35:45 shearer_lu Exp $ */
/* Project :  miniupnp
 * Website :  http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * Author :   Thomas Bernard
 * Copyright (c) 2005 Thomas Bernard
 * This software is subject to the conditions detailed in the
 * LICENCE file included in this distribution.
 * */
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <syslog.h>

#include "maco.h"
#include "minissdp.h"
#include "upnphttp.h"
#include "upnpevent.h"
#include "upnphttp_func.h"
#include "upnpdescgen.h"
#include "upnpsoap.h"


struct http_desc *upnp_http_desc = NULL;

struct upnphttp * 
New_upnphttp(int s)
{
	struct upnphttp * ret;
	if(s<0)
		return NULL;
	ret = (struct upnphttp *)malloc(sizeof(struct upnphttp));
	if(ret == NULL)
		return NULL;
	memset(ret, 0, sizeof(struct upnphttp));
	ret->socket = s;
	return ret;
}

void
CloseSocket_upnphttp(struct upnphttp * h)
{
	if(close(h->socket) < 0)
	{
		//syslog(LOG_ERR, "CloseSocket_upnphttp: close(%d): %m", h->socket);
	}
	h->socket = -1;
	h->state = 100;
}

void
Delete_upnphttp(struct upnphttp * h)
{
	if(h)
	{
		if(h->socket >= 0)
			close(h->socket);
		if(h->req_buf)
			free(h->req_buf);
		if(h->res_buf)
			free(h->res_buf);
		free(h);
	}
}

/* parse HttpHeaders of the REQUEST */
static void
ParseHttpHeaders(struct upnphttp * h)
{
	char * line;
	char * colon;
	char * p;//,tmp_str[64]={0};
	int n;
	
	line = h->req_buf;
	/* TODO : check if req_buf, contentoff are ok */
	while(line < (h->req_buf + h->req_contentoff))
	{
		colon = strchr(line, ':');
		if(colon)
		{
			if(strncasecmp(line, "Content-Length", 14)==0)
			{
				p = colon;
				while(*p < '0' || *p > '9')
					p++;
				h->req_contentlen = atoi(p);
				/*printf("*** Content-Lenght = %d ***\n", h->req_contentlen);
				printf("    readbufflen=%d contentoff = %d\n",
					h->req_buflen, h->req_contentoff);*/
			}
			else if(strncasecmp(line, "SOAPAction", 10)==0)
			{
				p = colon;
				n = 0;
				while(*p == ':' || *p == ' ' || *p == '\t')
					p++;
				while(p[n]>=' ')
				{
					n++;
				}
				if((p[0] == '"' && p[n-1] == '"')
				  || (p[0] == '\'' && p[n-1] == '\''))
				{
					p++; n -= 2;
				}
				h->req_soapAction = p;
				h->req_soapActionLen = n;
			}
#if 0			
			else if(strncasecmp(line, "Connection", 10)==0)
			{
				p = colon;
				n = 0;
				while(*p == ':' || *p == ' ' || *p == '\t')
					p++;
				while(p[n]>=' ' && p[n]!='\r')
				{
					n++;
				}		
				if(n<sizeof(tmp_str)){
					strcpy(tmp_str, p);
					tmp_str[n]=0;
				}			
#ifdef DEBUG
				printf("%s[%d] : Connection: %s\n",__FUNCTION__,__LINE__,tmp_str);
#endif						
			}		
#endif
			
		}
		while(!(line[0] == '\r' && line[1] == '\n'))
			line++;
		line += 2;
	}
}

/* very minimalistic 404 error message */
static void
Send404(struct upnphttp * h)
{
	static const char error404[] = "HTTP/1.1 404 Not found\r\n"
		"Connection: close\r\n"
		"Content-type: text/html\r\n"
		"\r\n"
		"<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD>"
		"<BODY><H1>Not Found</H1>The requested URL was not found"
		" on this server.</BODY></HTML>\r\n";
	int n;
	n = send(h->socket, error404, sizeof(error404) - 1, 0);
	if(n < 0)
	{
		//syslog(LOG_ERR, "Send404: send(http): %m");
	}
	CloseSocket_upnphttp(h);
}

/* very minimalistic 501 error message */
void
Send501(struct upnphttp * h)
{
	static const char error501[] = "HTTP/1.1 501 Not Implemented\r\n"
		"Connection: close\r\n"
		"Content-type: text/html\r\n"
		"\r\n"
		"<HTML><HEAD><TITLE>501 Not Implemented</TITLE></HEAD>"
		"<BODY><H1>Not Implemented</H1>The HTTP Method "
		"is not implemented by this server.</BODY></HTML>\r\n";
	int n;
	n = send(h->socket, error501, sizeof(error501) - 1, 0);
	if(n < 0)
	{
		//syslog(LOG_ERR, "Send501: send(http): %m");
	}
	CloseSocket_upnphttp(h);
}

/* very minimalistic 412 error message */
void
Send412(struct upnphttp * h)
{
	static const char error_str[] = "HTTP/1.1 412 Precondition Failed\r\n"
		"Server: Linux UPnP/1.0 miniupnpd/1.0\r\n"
		"CONNECTION: close\r\n"
		"CONTENT-TYPE: text/html\r\n"
		"<html><body><h1>412 Precondition Failed</h1></body></html>"
		"\r\n"
		;
	int n;
	n = send(h->socket, error_str, sizeof(error_str) - 1, 0);
	if(n < 0)
	{
		//syslog(LOG_ERR, "Send501: send(http): %m");
	}
	CloseSocket_upnphttp(h);
}

static const char *
findendheaders(const char * s, int len)
{
	while(len-->0)
	{
		if(s[0]=='\r' && s[1]=='\n' && s[2]=='\r' && s[3]=='\n')
			return s;
		s++;
	}
	return NULL;
}

#if 0
#ifdef DEBUG
static void
sendDummyDesc(struct upnphttp * h)
{
	static const char xml_desc[] = "<?xml version=\"1.0\"?>\n"
		"<scpd xmlns=\"urn:schemas-upnp-org:service-1-0\">"
		" <specVersion>"
		"    <major>1</major>"
		"    <minor>0</minor>"
		"  </specVersion>"
		"  <actionList />"
		"  <serviceStateTable />"
		"</scpd>";
	BuildResp_upnphttp(h, xml_desc, sizeof(xml_desc)-1);
	SendResp_upnphttp(h);
	CloseSocket_upnphttp(h);
}
#endif
#endif

/* Sends the description generated by the parameter */
static void
sendXMLdesc(struct upnphttp * h, char * (f)(int *))
{
	char * desc;
	int len;
	desc = f(&len);
	if(!desc)
	{
		//syslog(LOG_ERR, "Failed to generate XML description");
		return;
	}
	BuildResp_upnphttp(h, desc, len);
	SendResp_upnphttp(h);
	CloseSocket_upnphttp(h);
	free(desc);
}

/* ProcessHTTPPOST_upnphttp()
 * executes the SOAP query if it is possible */
static void
ProcessHTTPPOST_upnphttp(struct upnphttp * h)
{
	if((h->req_buflen - h->req_contentoff) >= h->req_contentlen)
	{
		if(h->req_soapAction)
		{
			/* we can process the request */
			//syslog(LOG_INFO, "SOAPAction: %.*s",
		    	   //h->req_soapActionLen, h->req_soapAction);
			ExecuteSoapAction(h, 
				h->req_soapAction,
				h->req_soapActionLen);
		}
		else
		{
			static const char err400str[] =
				"<html><body>Bad request</body></html>";
			//syslog(LOG_INFO, "No SOAPAction in HTTP headers");
			BuildResp2_upnphttp(h, 400, "Bad Request",
			                    err400str, sizeof(err400str) - 1);
			SendResp_upnphttp(h);
			CloseSocket_upnphttp(h);
		}
	}
	else
	{
		/* waiting for remaining data */
		h->state = 1;
	}
}

/* Parse and process Http Query 
 * called once all the HTTP headers have been received. */
static void
ProcessHttpQuery_upnphttp(struct upnphttp * h, struct eventlisthead *pevent_handlehead, struct event_list *list)
{
	char HttpCommand[16];
	char HttpUrl[128];
	char * HttpVer;
	char * p;
	int i;
	p = h->req_buf;
	if(!p)
		return;
	for(i = 0; i<15 && *p != ' ' && *p != '\r'; i++)
		HttpCommand[i] = *(p++);
	HttpCommand[i] = '\0';
	while(*p==' ')
		p++;
	for(i = 0; i<127 && *p != ' ' && *p != '\r'; i++)
		HttpUrl[i] = *(p++);
	HttpUrl[i] = '\0';
	while(*p==' ')
		p++;
	HttpVer = h->HttpVer;
	for(i = 0; i<15 && *p != '\r'; i++)
		HttpVer[i] = *(p++);
	HttpVer[i] = '\0';
#ifdef DEBUG		
	printf("%s[%d] : HttpVer=%s\n",__FILE__,__LINE__, HttpVer);
#endif		
	//syslog(LOG_INFO, "HTTP REQUEST : %s %s (%s)",
	       //HttpCommand, HttpUrl, HttpVer);
	ParseHttpHeaders(h);
	if(strcmp("POST", HttpCommand) == 0)
	{
		h->req_command = EPost;
		ProcessHTTPPOST_upnphttp(h);
	}
	else if(strcmp("GET", HttpCommand) == 0)
	{
		int i = -1;
		h->req_command = EGet;
		/* it is  igd_http_desc*/
		while(upnp_http_desc[++i].path != NULL)
		{
			if(strcmp(upnp_http_desc[i].path,HttpUrl) == 0)
			{
#ifdef DEBUG		
		printf("%s[%d] : sendXMLdesc\n",__FILE__,__LINE__);
#endif				
				sendXMLdesc(h, upnp_http_desc[i].gen_desc);
				return ;
			}			
		}
		
		//syslog(LOG_NOTICE, "%s not found, responding ERROR 404", HttpUrl);
		Send404(h);
	}
	else if(strcmp("SUBSCRIBE", HttpCommand) == 0)
	{
		//syslog(LOG_NOTICE, "call SUBSCRIBE function");
#ifdef DEBUG		
		printf("%s[%d] : call SUBSCRIBE function; \n",__FILE__,__LINE__);
#endif
		handle_subcribe(h,pevent_handlehead,list);
	}
	else if(strcmp("UNSUBSCRIBE", HttpCommand) == 0)
	{
		//syslog(LOG_NOTICE, "call UNSUBSCRIBE function");
#ifdef DEBUG		
		printf("%s[%d] : call UNSUBSCRIBE function; \n",__FILE__,__LINE__);
#endif
		handle_unsubcribe(h,pevent_handlehead,list);
	}
	else
	{
		//syslog(LOG_NOTICE, "Unsupported HTTP Command %s", HttpCommand);
		printf("%s[%d] : Unsupported HTTP Command %s; \n",__FILE__,__LINE__,HttpCommand);
		Send501(h);
	}
}


void
Process_upnphttp(struct upnphttp * h, struct eventlisthead *pevent_handlehead, struct event_list *list)
{
	char buf[2048];
	int n;
	if(!h)
		return;
	switch(h->state)
	{
	case 0:
#ifdef DEBUG
		printf("%s[%d] : state is 0: \n",__FILE__,__LINE__);
#endif
		n = recv(h->socket, buf, 2048, 0);
		if(n<0)
		{
			//syslog(LOG_ERR, "recv (state0): %m");
			h->state = 100;
		}
		else if(n==0)
		{
			//syslog(LOG_WARNING, "HTTP Connection closed inexpectedly");
			h->state = 100;
		}
		else
		{
			const char * endheaders;
			/* if 1st arg of realloc() is null,
			 * realloc behaves the same as malloc() */
			h->req_buf = (char *)realloc(h->req_buf, n + h->req_buflen + 1);
			memcpy(h->req_buf + h->req_buflen, buf, n);
			h->req_buflen += n;
			h->req_buf[h->req_buflen] = '\0';
			/* search for the string "\r\n\r\n" */
			endheaders = findendheaders(h->req_buf, h->req_buflen);
			if(endheaders)
			{
				h->req_contentoff = endheaders - h->req_buf + 4;
				ProcessHttpQuery_upnphttp(h,pevent_handlehead,list);
			}
		}
		break;
	case 1:
		/* This is used to process POST CMD, contain more than one packets */
#ifdef DEBUG
		printf("%s[%d] : state is 1: \n",__FILE__,__LINE__);
#endif
		n = recv(h->socket, buf, 2048, 0);
		if(n<0)
		{
			//syslog(LOG_ERR, "recv (state1): %m");
			h->state = 100;
		}
		else if(n==0)
		{
			//syslog(LOG_WARNING, "HTTP Connection closed inexpectedly");
			h->state = 100;
		}
		else
		{
			/*fwrite(buf, 1, n, stdout);*/	/* debug */
			h->req_buf = (char *)realloc(h->req_buf, n + h->req_buflen+1);
			memcpy(h->req_buf + h->req_buflen, buf, n);
			h->req_buflen += n;
			h->req_buf[h->req_buflen]='\0';
			if((h->req_buflen - h->req_contentoff) >= h->req_contentlen)
			{
				ProcessHTTPPOST_upnphttp(h);
			}
		}
		break;
	default:
		;
		//syslog(LOG_WARNING, "Unexpected state: %d", h->state);
	}
}

static const char httpresphead[] =
	"%s %d %s\r\n"
	"Content-Type: text/xml; charset=\"utf-8\"\r\n"
	"Connection: close\r\n"
	"Content-Length: %d\r\n"
	/*"Server: miniupnpd/1.0 UPnP/1.0\r\n"*/
	"Server: " MINIUPNPD_SERVER_STRING "\r\n"
	"Ext:\r\n"
	"\r\n";
/*
		"<?xml version=\"1.0\"?>\n"
		"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
		"s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
		"<s:Body>"

		"</s:Body>"
		"</s:Envelope>";
*/
/* with response code and response message
 * also allocate enough memory */

void
BuildHeader_upnphttp(struct upnphttp * h, int respcode,
                     const char * respmsg,
                     int bodylen)
{
	int templen;
	if(!h->res_buf)
	{
		templen = sizeof(httpresphead) + 64 + bodylen;
		h->res_buf = (char *)malloc(templen);
		h->res_buf_alloclen = templen;
	}
	h->res_buflen = snprintf(h->res_buf, h->res_buf_alloclen,
	                         httpresphead, h->HttpVer,
	                         respcode, respmsg, bodylen);
	if(h->res_buf_alloclen < (h->res_buflen + bodylen))
	{
		h->res_buf = (char *)realloc(h->res_buf, (h->res_buflen + bodylen));
		h->res_buf_alloclen = h->res_buflen + bodylen;
	}
}

void
BuildResp2_upnphttp(struct upnphttp * h, int respcode,
                    const char * respmsg,
                    const char * body, int bodylen)
{
	BuildHeader_upnphttp(h, respcode, respmsg, bodylen);
	memcpy(h->res_buf + h->res_buflen, body, bodylen);
	h->res_buflen += bodylen;
}

/* responding 200 OK ! */
void
BuildResp_upnphttp(struct upnphttp * h,
                        const char * body, int bodylen)
{
	BuildResp2_upnphttp(h, 200, "OK", body, bodylen);
}

void
SendResp_upnphttp(struct upnphttp * h)
{
	int n;
	n = send(h->socket, h->res_buf, h->res_buflen, 0);
	if(n<0)
	{
		//syslog(LOG_ERR, "send(res_buf): %m");
	}
	else if(n < h->res_buflen)
	{
		/* TODO : handle correctly this case */
		//syslog(LOG_ERR, "send(res_buf): %d bytes sent (out of %d)",
						//n, h->res_buflen);
	}
}

