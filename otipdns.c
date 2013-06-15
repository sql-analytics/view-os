/*   
 *   otipdns.c: One Time IP DNS forwarder
 *   
 *   Copyright 2012 Renzo Davoli - Virtual Square Team 
 *   University of Bologna - Italy
 *   
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stddef.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <lwipv6.h>
#include <poll.h>
#include <mhash.h>
#include <getopt.h>
#include <syslog.h>
#include <stdarg.h>
#define BUFSIZE 1024

#ifndef LWIP_STACK_FLAG_NO_ECHO
#define LWIP_STACK_FLAG_NO_ECHO 0
#endif

// #define TIMEOUT 90
#define TIMEOUT 10
#define TAG "__"
#define TAGLEN 2

static int logok=0;
static int daemonize; // daemon mode
static int verbose;
static int noecho;
static char pidfile_path[PATH_MAX];
static char *pidfile;
static char *switch_path;
static char *progname;

struct request {
	unsigned short id;
	unsigned short    clientid;
	struct sockaddr_in6 from;
	time_t	reqtime;
	unsigned char *query;

	struct request *next;
};

unsigned short myid;
struct request *rhead;

void rq_add(struct request *new)
{
	//printf("ADD %x %x\n",new->id,new->clientid);
	new->next=rhead;
	rhead=new;
}

static struct request *rq_search_detach_r(struct request **head, unsigned short myid)
{
	if (*head) {
		if ((*head)->id == myid) {
			struct request *rv=(*head);
			*head=(*head)->next;
			return rv;
		} else 
			return rq_search_detach_r(&((*head)->next),myid);
	} else
		return NULL;
}

struct request *rq_search_detach(unsigned short myid)
{
	//printf("DEL %d\n",myid);
	return rq_search_detach_r(&rhead,myid);
}

static struct request * rq_clean_r(struct request *head, time_t now)
{
	if (head) {
		//printf("CHK %d\n",head->id);
		if (head->reqtime + TIMEOUT < now) {
			struct request *next=rq_clean_r(head->next, now);
			if (head->query) free(head->query);
			free(head);
			return next;
		} else {
			head->next=rq_clean_r(head->next, now);
			return head;
		}
	} else
		return NULL;
}

void rq_clean(time_t now) 
{
	rhead=rq_clean_r(rhead, now);
}

void create_mask(struct ip_addr *addr, struct ip_addr *mask, int bits)
{
	int i;
	int len=sizeof(*mask);
	unsigned char *maskc=(unsigned char *)mask;
	if (IP_ADDR_IS_V4(addr)) 
		bits+=96;
	for(i=0;i<len;i++,bits -= 8) {
		if (bits >= 8)
			maskc[i]=255;
		else if (bits <= 0)
			maskc[i]=0;
		else
			maskc[i]=256 - (1<<(8-bits));
	}
}

unsigned char *skipname(unsigned char *s)
{
	//printf("skipname %s\n",s);
	if (*s==0) return s+1;
	if ((*s & 0xc0) == 0xc0) return s+2;
	return skipname(s+ (*s+1));
}

unsigned char *pluspoint(unsigned char *s)
{
	int i;
	if (*s==0) return NULL;
	if ((*s && 0xc0) == 0xc0) return NULL;
	for (i=0;i< *s; i++) 
		if (strncmp(&s[i+1],TAG,TAGLEN)==0) 
		{
			*s = i; s[i+1] = 0;
			return s+(i+1);
		}
	return pluspoint(s+ (*s+1));
}

unsigned char *decodepwd(unsigned char *s)
{
	unsigned char *pwd;
	int i;
	if (*s==0) return NULL;
	if ((*s && 0xc0) == 0xc0) return NULL;
	pwd=decodepwd(s+ (*s+1));
	for (i=0;i< *s; i++)
		if (strncmp(&s[i+1],TAG,TAGLEN)==0)
			pwd=s+(i+TAGLEN+1);
	*s='.';
	return pwd;
}

void getpwd(unsigned char *s)
{
	unsigned char *pwd;
	pwd=decodepwd(s);
	//printf("decodefqh %s=%s\n",pwd,s);
	memmove(s,pwd,strlen(pwd)+1);
	//printf("PASSWD=%s\n",s);
}


void computeaddr(unsigned char *saddr,unsigned char *pwd) {
	struct ip_addr *addr=(struct ip_addr *) saddr;
	char *s;
	MHASH td;
	unsigned char out[mhash_get_block_size(MHASH_MD5)];
	time_t now;
	int len=asprintf(&s,"%d %s",time(&now) >> 6,pwd);
	td=mhash_init(MHASH_MD5);
	mhash(td, s, len);
	mhash_deinit(td, out);
	free(s);

#if 0
	printf("%x:%x:%x:%x:%x:%x:%x:%x\n",
			ntohl(addr->addr[0]) >> 16, ntohl(addr->addr[0]) & 0xffff,
			ntohl(addr->addr[1]) >> 16, ntohl(addr->addr[1]) & 0xffff,
			ntohl(addr->addr[2]) >> 16, ntohl(addr->addr[2]) & 0xffff,
			ntohl(addr->addr[3]) >> 16, ntohl(addr->addr[3]) & 0xffff);
#endif
	addr->addr[2] ^= htonl(
			((out[0] ^ out[8]) << 24) |
			((out[1] ^ out[9]) << 16) |
			((out[2] ^ out[10]) << 8) |
			(out[3] ^ out[11]));
	addr->addr[3] ^= htonl(
			((out[4] ^ out[12]) << 24) |
			((out[5] ^ out[13]) << 16) |
			((out[6] ^ out[14]) << 8) |
			(out[7] ^ out[15]));
#if 0
	printf("%x:%x:%x:%x:%x:%x:%x:%x\n",
			ntohl(addr->addr[0]) >> 16, ntohl(addr->addr[0]) & 0xffff,
			ntohl(addr->addr[1]) >> 16, ntohl(addr->addr[1]) & 0xffff,
			ntohl(addr->addr[2]) >> 16, ntohl(addr->addr[2]) & 0xffff,
			ntohl(addr->addr[3]) >> 16, ntohl(addr->addr[3]) & 0xffff);
#endif
}

int parse_dns_otip_req(unsigned char *buf, int len, struct request *req)
{
	static unsigned char isaquery[]={0x01,0x00,0x00,0x01};
	static unsigned char isaaaa[]={0x00,0x1c};
	if (memcmp(buf+2,isaquery,sizeof(isaquery))==0) {
		unsigned char *query=buf+12;
		unsigned char *querytail=skipname(query);
		unsigned char *plus;
		int querylength=(querytail-query)+4;
		//printf ("IS AAAA %d\n", (memcmp(querytail,isaaaa,sizeof(isaaaa))==0));
		//printf ("QUERYLENGTH=%d\n",(querytail-query)+4);
		if ((memcmp(querytail,isaaaa,sizeof(isaaaa))==0)) {
			int i;
			req->query=malloc(querylength);
			memcpy(req->query,query,querylength);
			if ((plus=pluspoint(query)) != NULL) {
				int deltalen=querytail-(plus+1);
				memmove(plus+1,querytail,len-(querytail-buf));
				//printf("PLUS! deltalen %d %d\n",len,deltalen);
				len -= deltalen;
			}
		}
	}
	return len;
}

#define TAILCUT
int parse_dns_otip_reply(unsigned char *buf, int len, struct request *req)
{
	static unsigned char isaaaa[]={0x00,0x1c};
	static unsigned char isaaaaIN[]={0x00,0x1c,0x00,0x01};
	static unsigned char ttlmin[]={0x00,0x00,0x00,0x01};
	//printf("LEN %d\n",len);
	if (buf[2] & 0x80) {
		int nreq=(buf[4]<<8)+buf[5];
		int nRR=(buf[6]<<8)+buf[7];
#ifdef TAILCUT
		buf[8]=buf[9]=buf[10]=buf[11]=0;
#endif
		//printf("reply: nreq=%d nRR=%d\n",nreq,nRR);
		unsigned char *query=buf+12;
		unsigned char *querytail=skipname(query);
		int querylength=(querytail-query)+4;
		//printf ("IS AAAA %d\n", (memcmp(querytail,isaaaa,sizeof(isaaaa))==0));
		//printf ("QUERYLENGTH=%d\n",(querytail-query)+4);
		if (req->query) {
			unsigned char *requerytail=skipname(req->query);
			int requerylength=(requerytail-req->query)+4;
			int i;
			unsigned char *reply;
			//printf ("REQUERYLENGTH=%d\n",(requerytail-req->query)+4);
			if (requerylength>querylength) {
				int deltalen=requerylength-querylength;
				//printf("PLUS REPLY %d %d %d\n",len,querytail-buf,len-(querytail-buf)-4);
				memmove(querytail+(deltalen+4),querytail+4,len-(querytail-buf)-4);
				memcpy(query,req->query,requerylength);
				len+=deltalen;
				getpwd(req->query);
				reply=query+requerylength;
				//printf("REPLY %d\n",reply-buf);
				/* skip the other requests */
				for (i=1; i<nreq; i++) {
					reply=skipname(reply);
					reply+=4;
				}
				for (i=0; i<nRR; i++) {
					unsigned char *tailreply=skipname(reply);
					int len = (tailreply[8] << 8) + tailreply[9];
					if (memcmp(tailreply,isaaaaIN,sizeof(isaaaaIN))==0 && len==16) {
						memcpy(tailreply+4,ttlmin,sizeof(ttlmin));
						computeaddr(tailreply+10,req->query);
					}
					reply = tailreply + (len + 10);
				}
#ifdef TAILCUT
				len=reply-buf;
#endif
			}
		}
	}
	//printf("FINALLEN %d\n",len);
	return len;
}
#if 0
void print_s6(struct sockaddr_in6 *s)
{
	char host[1024];
	char service[1024];
	getnameinfo((struct sockaddr *)s,sizeof(*s),host,sizeof(host),service,sizeof(service),NI_NUMERICHOST|NI_NUMERICSERV);
	//printf("%s %s\n",host,service);
}
#endif

void usage()
{
	fprintf(stderr,"Usage: %s OPTIONS DNSvirtserver[/mask]  RealDNS [switch]\n"
			"\t--help|-h\n"
			"\t--daemon|-d\n"
			"\t--pidfile pidfile\n"
			"\t--verbose|-v\n",
			"\t--noecho|-n\n",
			"\t--sock|--switch|-s vde_switch\n",
			progname);
	exit(1);
}

void printlog(int priority, const char *format, ...)
{
	va_list arg;

	va_start (arg, format);

	if (logok)
		vsyslog(priority,format,arg);
	else {
		fprintf(stderr,"%s: ",progname);
		vfprintf(stderr,format,arg);
		fprintf(stderr,"\n");
	}
	va_end (arg);
}

static void save_pidfile()
{
	if(pidfile[0] != '/')
		strncat(pidfile_path, pidfile, PATH_MAX - strlen(pidfile_path));
	else
		strcpy(pidfile_path, pidfile);

	int fd = open(pidfile_path,
			O_WRONLY | O_CREAT | O_EXCL,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	FILE *f;

	if(fd == -1) {
		printlog(LOG_ERR, "Error in pidfile creation: %s", strerror(errno));
		exit(1);
	}

	if((f = fdopen(fd, "w")) == NULL) {
		printlog(LOG_ERR, "Error in FILE* construction: %s", strerror(errno));
		exit(1);
	}

	if(fprintf(f, "%ld\n", (long int)getpid()) <= 0) {
		printlog(LOG_ERR, "Error in writing pidfile");
		exit(1);
	}

	fclose(f);
}

/* ./otipdns 2001:760:2e00:ff00::42 192.168.254.1
	           DNS v server [/mask]   Real DNS      [switch] */
int main(int argc, char *argv[])
{
	struct addrinfo hints;
	struct addrinfo *vaddr, *saddr;
	int err;
	char *smask;
	int maskbits;
	struct stack *stack;
	struct netif *netif;
	struct ip_addr addr;
	struct ip_addr mask;
	struct sockaddr_in6 *vsockaddr, *ssockaddr, lsockaddr;
	int vsock, ssock;
  unsigned char buf[2*BUFSIZE];
	int n;
	time_t now;
#define PIDFILEARG 131
	static struct option long_options[] = {
		{"help",0 , 0, 'h'},
		{"daemon",0 , 0, 'd'},
		{"pidfile", 1, 0, PIDFILEARG},
		{"verbose",0 , 0, 'v'},
		{"noecho",0 , 0, 'n'},
		{"switch",1 , 0, 's'},
		{"sock",1 , 0, 's'},
		{0,0,0,0}
	};
	int option_index;
	progname=argv[0];
	while(1) {
		int c;
		c = getopt_long (argc, argv, "hdvns:",
				long_options, &option_index);
		if (c<0)
			break;
		switch (c) {
			case 'h':
				usage();
				break;
			case 'd':
				daemonize=1;
				break;
			case 'v':
				verbose=1;
				break;
			case 'n':
				noecho=1;
				break;
			case 's':
				switch_path=strdup(optarg);
				break;
			case PIDFILEARG:
				pidfile=strdup(optarg);
				break;
			default:
				usage();
				break;
		}
	}

	if (argc-optind == 3 && switch_path==NULL) {
		switch_path=strdup(argv[optind+2]);
		argc--;
	}
	if (argc-optind != 2)
		usage();

	if ((smask=strchr(argv[optind],'/'))==NULL) 
		maskbits=64;
	else {
		maskbits=atoi(smask+1);
		*smask=0;
	}

	if (daemonize) {
		openlog(basename(progname), LOG_PID, 0);
		logok=1;
		syslog(LOG_INFO,"%s started",progname);
	}
	/* saves current path in pidfile_path, because otherwise with daemonize() we
	 * forget it */
	if(getcwd(pidfile_path, PATH_MAX-1) == NULL) {
		printlog(LOG_ERR, "getcwd: %s", strerror(errno));
		exit(1);
	}
	strcat(pidfile_path, "/");
	if (daemonize && daemon(0, 0)) {
		printlog(LOG_ERR,"daemon: %s",strerror(errno));
		exit(1);
	}

	/* once here, we're sure we're the true process which will continue as a
	 * server: save PID file if needed */
	if(pidfile) save_pidfile();

	memset(&hints,0,sizeof(struct addrinfo));
	hints.ai_family = AF_INET6;
	hints.ai_flags = (AI_V4MAPPED | AI_ADDRCONFIG);
	if ((err=getaddrinfo(argv[optind],"domain",&hints,&vaddr)) != 0) {
		printlog(LOG_ERR,"%s- err: %s %s",progname,argv[optind],gai_strerror(err));
		return 1;
	}
	if ((err=getaddrinfo(argv[optind+1],"domain",&hints,&saddr)) != 0) {
		printlog(LOG_ERR,"%s- err: %s %s",progname,argv[optind+1],gai_strerror(err));
		return 1;
	}
	vsockaddr=(struct sockaddr_in6 *)vaddr->ai_addr;
	ssockaddr=(struct sockaddr_in6 *)saddr->ai_addr;

	if (vsockaddr->sin6_family != AF_INET6 ||
			ssockaddr->sin6_family != AF_INET6) {
		printlog(LOG_ERR,"%s- err GAI ERR IPv6",progname);
		return 1;
	}

	memset(&lsockaddr, 0, sizeof(struct sockaddr_in6));
	lsockaddr.sin6_family = AF_INET6;

	stack=lwip_add_stack(noecho?LWIP_STACK_FLAG_NO_ECHO:0);
	netif = lwip_add_vdeif(stack, switch_path, NETIF_STD_FLAGS);

	memcpy(&addr,&vsockaddr->sin6_addr,sizeof(addr));

	lwip_ifup_flags(netif, 1);
	create_mask(&addr,&mask,maskbits);
	lwip_add_addr(netif,&addr,&mask);
	vsock=lwip_msocket(stack,vsockaddr->sin6_family,SOCK_DGRAM,0);
	ssock=socket(ssockaddr->sin6_family,SOCK_DGRAM,0);
	lwip_bind(vsock,(struct sockaddr *)vsockaddr,sizeof(struct sockaddr_in6));
	bind(ssock,(struct sockaddr *)&lsockaddr,sizeof(struct sockaddr_in6));
	connect(ssock,(struct sockaddr *)ssockaddr,sizeof(struct sockaddr_in6));
	while (1) {
		struct pollfd pfd[2]={{vsock,POLLIN|POLLHUP,0},{ssock,POLLIN|POLLHUP,0}};
		lwip_poll(pfd,2,1000);
		time(&now);
		if ((pfd[0].revents & POLLHUP) || (pfd[1].revents & POLLHUP))
			break;
		if (pfd[0].revents & POLLIN) {
			struct sockaddr_in6 from;
			struct request *req;

			socklen_t fromlen=sizeof(from);
			n=lwip_recvfrom(vsock,buf,BUFSIZE,0,(struct sockaddr *)&from,&fromlen);
			if (n==0) break;
			//printf("0->%s\n",buf+12);
			//print_s6(&from);
			if (n >= 12) {
				req = calloc(1, sizeof(struct request)); 
				if (req) {
					req->id = myid++;
					memcpy(&req->from, &from, sizeof(struct sockaddr_in6));
					memcpy(&req->clientid, buf, sizeof(unsigned short));
					memcpy(buf, &req->id, sizeof(unsigned short));
					req->reqtime=now;
					req->query=NULL;
					n=parse_dns_otip_req(buf,n,req);

					rq_add (req);

					send(ssock,buf,n,0);
				}
			}
		}
		if (pfd[1].revents & POLLIN) {
			struct sockaddr_in6 to;
			unsigned short myid;
			struct request *req;
			n=recv(ssock,buf,BUFSIZE,0);
			if (n==0) break;
			//printf("1->%s %d\n",buf+12,n);
			if (n >= 12) {
				memcpy(&myid, buf, sizeof(unsigned short));
				req=rq_search_detach(myid);

				if (req) {
					memcpy(buf,&req->clientid, sizeof(unsigned short));
					if (n>12)
						n=parse_dns_otip_reply(buf,n,req);
					lwip_sendto(vsock,buf,n,0,
							(struct sockaddr *)&req->from,sizeof(struct sockaddr_in6));
					if (req->query) free(req->query);
					free(req);
				}
			}
		}
		rq_clean(now);
	}
	close(ssock);
	lwip_close(vsock);
	return 0;
}


