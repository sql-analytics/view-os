/*   This is part of LWIPv6
 *   Developed for the Ale4NET project
 *   Application Level Environment for Networking
 *   
 *   Copyright 2004 Renzo Davoli University of Bologna - Italy
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
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */   
/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 */


#ifndef __LWIP_SOCKETS_H__
#define __LWIP_SOCKETS_H__

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>

#include "lwip/ip_addr.h"
#if 0
struct stack;
struct in_addr {
          u32_t s_addr;
};

#define INADDR_NONE    ((u32_t) 0xffffffff)  /* 255.255.255.255 */

struct in6_addr {
        unsigned char   s6_addr[16];/* IPv6 address */
};

struct sockaddr_in {
  u16_t sin_family;
  u16_t sin_port;
  struct in_addr sin_addr;
  char sin_zero[8];
};

struct sockaddr_in6 {
  u16_t sin6_family;
  u16_t sin6_port;
  u_int32_t       sin6_flowinfo;
  struct in6_addr sin6_addr;
  u_int32_t   sin6_scope_id; 
};

struct sockaddr {
  u16_t sa_family;
  char sa_data[14];
};

#ifndef socklen_t
#  define socklen_t int
#endif

struct msghdr {
	void         *msg_name;       /* optional address */
	socklen_t     msg_namelen;    /* size of address */
	struct iovec *msg_iov;        /* scatter/gather array */
	size_t        msg_iovlen;     /* # elements in msg_iov */
	void         *msg_control;    /* ancillary data, see below */
	socklen_t     msg_controllen; /* ancillary data buffer len */
	int           msg_flags;      /* flags on received message */
};


#define SOCK_STREAM     1
#define SOCK_DGRAM      2
#define SOCK_RAW        3

/*
 * Option flags per-socket.
 */

#define SO_DEBUG        1
#define SO_REUSEADDR    2
#define SO_TYPE         3
#define SO_ERROR        4
#define SO_DONTROUTE    5
#define SO_BROADCAST    6
#define SO_SNDBUF       7
#define SO_RCVBUF       8
#define SO_KEEPALIVE    9
#define SO_OOBINLINE    10
#define SO_NO_CHECK     11
#define SO_PRIORITY     12
#define SO_LINGER       13
#define SO_BSDCOMPAT    14
#define SO_REUSEPORT    15 
#define SO_RCVLOWAT     16
#define SO_SNDLOWAT     17
#define SO_RCVTIMEO     18
#define SO_SNDTIMEO     19
#define SO_PASSCRED     20
#define SO_PEERCRED     21
#define SO_SECURITY_AUTHENTICATION              22
#define SO_SECURITY_ENCRYPTION_TRANSPORT        23
#define SO_SECURITY_ENCRYPTION_NETWORK          24
#define SO_BINDTODEVICE 25
#define SO_ATTACH_FILTER        26
#define SO_DETACH_FILTER        27
#define SO_PEERNAME             28
#define SO_TIMESTAMP            29
#define SCM_TIMESTAMP           SO_TIMESTAMP
#define SO_ACCEPTCONN           30

#if 0
#define  SO_DEBUG  0x0001    /* turn on debugging info recording */
#define  SO_ACCEPTCONN  0x0002    /* socket has had listen() */
#define  SO_REUSEADDR  0x0004    /* allow local address reuse */
#define  SO_KEEPALIVE  0x0008    /* keep connections alive */
#define  SO_DONTROUTE  0x0010    /* just use interface addresses */
#define  SO_BROADCAST  0x0020    /* permit sending of broadcast msgs */
#define  SO_USELOOPBACK  0x0040    /* bypass hardware when possible */
#define  SO_LINGER  0x0080    /* linger on close if data present */
#define  SO_OOBINLINE  0x0100    /* leave received OOB data in line */
#define	 SO_REUSEPORT	0x0200		/* allow local address & port reuse */

#define SO_DONTLINGER   (int)(~SO_LINGER)

/*
 * Additional options, not kept in so_options.
 */
#define SO_SNDBUF  0x1001    /* send buffer size */
#define SO_RCVBUF  0x1002    /* receive buffer size */
#define SO_SNDLOWAT  0x1003    /* send low-water mark */
#define SO_RCVLOWAT  0x1004    /* receive low-water mark */
#define SO_SNDTIMEO  0x1005    /* send timeout */
#define SO_RCVTIMEO  0x1006    /* receive timeout */
#define  SO_ERROR  0x1007    /* get error status and clear */
#define  SO_TYPE    0x1008    /* get socket type */
#endif



/*
 * Structure used for manipulating linger option.
 */
struct linger {
       int l_onoff;                /* option on/off */
       int l_linger;               /* linger time */
};

/*
 * Level number for (get/set)sockopt() to apply to socket itself.
 */
#define  SOL_SOCKET  0x1    /* options for socket level */
#if LWIP_PACKET
#define  SOL_PACKET  263
#endif

#define AF_UNSPEC       0
#define AF_INET         2
#define PF_INET         AF_INET
#define AF_INET6        10
#define PF_INET6        AF_INET6
#define AF_NETLINK      16
#define PF_NETLINK	AF_NETLINK
#define AF_PACKET       17
#define PF_PACKET	AF_PACKET
#define PF_UNSPEC       AF_UNSPEC

#define IPPROTO_IP      0
#define IPPROTO_TCP     6
#define IPPROTO_UDP     17
#define IPPROTO_IPV6    41
#define IPPROTO_ICMPV6  58
#define IPPROTO_RAW 	255

#define INADDR_ANY      0
#define INADDR_BROADCAST 0xffffffff

/* Flags we can use with send and recv. */
#define MSG_DONTWAIT    0x40            /* Nonblocking i/o for this operation only */


/*
 * Options for level IPPROTO_IP
 */
#define IP_TOS       1
#define IP_TTL       2
#define IP_HDRINCL   3
#define IP_MTU_DISCOVER    10 /* int */
#define IP_RECVERR         11 /* bool */
#define IP_RECVTTL         12 /* bool */
#define IP_RECVTOS         13 /* bool */


/*
 * Options for level IPPROTO_RAW
 */

#define IPV6_CHECKSUM 7

/*
 * Options for level IPPROTO_ICMPV6
 */
#define ICMPV6_FILTER 1

/*
 * Options for level IPPROTO_IPV6
 */
#define IPV6_HOPLIMIT 8
#define IPV6_UNICAST_HOPS 16
#define IPV6_MULTICAST_HOPS 18

#define IPTOS_TOS_MASK          0x1E
#define IPTOS_TOS(tos)          ((tos) & IPTOS_TOS_MASK)
#define IPTOS_LOWDELAY          0x10
#define IPTOS_THROUGHPUT        0x08
#define IPTOS_RELIABILITY       0x04
#define IPTOS_LOWCOST           0x02
#define IPTOS_MINCOST           IPTOS_LOWCOST

/*
 * Definitions for IP precedence (also in ip_tos) (hopefully unused)
 */
#define IPTOS_PREC_MASK                 0xe0
#define IPTOS_PREC(tos)                ((tos) & IPTOS_PREC_MASK)
#define IPTOS_PREC_NETCONTROL           0xe0
#define IPTOS_PREC_INTERNETCONTROL      0xc0
#define IPTOS_PREC_CRITIC_ECP           0xa0
#define IPTOS_PREC_FLASHOVERRIDE        0x80
#define IPTOS_PREC_FLASH                0x60
#define IPTOS_PREC_IMMEDIATE            0x40
#define IPTOS_PREC_PRIORITY             0x20
#define IPTOS_PREC_ROUTINE              0x00

/* FIX: not needed? */
///#include <asm/ioctls.h>

/*
 * Commands for ioctlsocket(),  taken from the BSD file fcntl.h.
 *
 *
 * Ioctl's have the command encoded in the lower word,
 * and the size of any in or out parameters in the upper
 * word.  The high 2 bits of the upper word are used
 * to encode the in/out status of the parameter; for now
 * we restrict parameters to at most 128 bytes.
 */
#if !defined(FIONREAD) || !defined(FIONBIO)
#define IOCPARM_MASK    0x7f            /* parameters must be < 128 bytes */
#define IOC_VOID        0x20000000      /* no parameters */
#define IOC_OUT         0x40000000      /* copy out parameters */
#define IOC_IN          0x80000000      /* copy in parameters */
#define IOC_INOUT       (IOC_IN|IOC_OUT)
                                        /* 0x20000000 distinguishes new &
                                           old ioctl's */
#define _IO(x,y)        (IOC_VOID|((x)<<8)|(y))

#define _IOR(x,y,t)     (IOC_OUT|(((long)sizeof(t)&IOCPARM_MASK)<<16)|((x)<<8)|(y))

#define _IOW(x,y,t)     (IOC_IN|(((long)sizeof(t)&IOCPARM_MASK)<<16)|((x)<<8)|(y))
#endif

#ifndef FIONREAD
#define FIONREAD    _IOR('f', 127, unsigned long) /* get # bytes to read */
#endif
#ifndef FIONBIO
#define FIONBIO     _IOW('f', 126, unsigned long) /* set/clear non-blocking i/o */
#endif

/* Socket I/O Controls */
#ifndef SIOCSHIWAT
#define SIOCSHIWAT  _IOW('s',  0, unsigned long)  /* set high watermark */
#define SIOCGHIWAT  _IOR('s',  1, unsigned long)  /* get high watermark */
#define SIOCSLOWAT  _IOW('s',  2, unsigned long)  /* set low watermark */
#define SIOCGLOWAT  _IOR('s',  3, unsigned long)  /* get low watermark */
#define SIOCATMARK  _IOR('s',  7, unsigned long)  /* at oob mark? */
#endif

#ifndef O_NONBLOCK
#define O_NONBLOCK    04000U
#endif

#ifndef FD_SET
  #undef  FD_SETSIZE
  #define FD_SETSIZE    16
  #define FD_SET(n, p)  ((p)->fd_bits[(n)/8] |=  (1 << ((n) & 7)))
  #define FD_CLR(n, p)  ((p)->fd_bits[(n)/8] &= ~(1 << ((n) & 7)))
  #define FD_ISSET(n,p) ((p)->fd_bits[(n)/8] &   (1 << ((n) & 7)))
  #define FD_ZERO(p)    memset((void*)(p),0,sizeof(*(p)))

  typedef struct fd_set {
          unsigned char fd_bits [(FD_SETSIZE+7)/8];
        } fd_set;

/* 
 * only define this in sockets.c so it does not interfere
 * with other projects namespaces where timeval is present
 */ 
#ifndef LWIP_TIMEVAL_PRIVATE
#define LWIP_TIMEVAL_PRIVATE 1
#endif

#if LWIP_TIMEVAL_PRIVATE
  struct timeval {
    long    tv_sec;         /* seconds */
    long    tv_usec;        /* and microseconds */
  };
#endif

#endif

/* other calls */
#define SIOCGSTAMP      0x8906          /* Get stamp */
/* Routing table calls.  */
#define SIOCADDRT       0x890B          /* add routing table entry      */
#define SIOCDELRT       0x890C          /* delete routing table entry   */
#define SIOCRTMSG       0x890D          /* call to routing system       */

/* Socket configuration controls. */
#define SIOCGIFNAME     0x8910          /* get iface name               */
#define SIOCSIFLINK     0x8911          /* set iface channel            */
#define SIOCGIFCONF     0x8912          /* get iface list               */
#define SIOCGIFFLAGS    0x8913          /* get flags                    */
#define SIOCSIFFLAGS    0x8914          /* set flags                    */
#define SIOCGIFADDR     0x8915          /* get PA address               */
#define SIOCSIFADDR     0x8916          /* set PA address               */
#define SIOCGIFDSTADDR  0x8917          /* get remote PA address        */
#define SIOCSIFDSTADDR  0x8918          /* set remote PA address        */
#define SIOCGIFBRDADDR  0x8919          /* get broadcast PA address     */
#define SIOCSIFBRDADDR  0x891a          /* set broadcast PA address     */
#define SIOCGIFNETMASK  0x891b          /* get network PA mask          */
#define SIOCSIFNETMASK  0x891c          /* set network PA mask          */
#define SIOCGIFMETRIC   0x891d          /* get metric                   */
#define SIOCSIFMETRIC   0x891e          /* set metric                   */
#define SIOCGIFMEM      0x891f          /* get memory address (BSD)     */
#define SIOCSIFMEM      0x8920          /* set memory address (BSD)     */
#define SIOCGIFMTU      0x8921          /* get MTU size                 */
#define SIOCSIFMTU      0x8922          /* set MTU size                 */
#define SIOCSIFNAME     0x8923          /* set interface name           */
#define SIOCSIFHWADDR   0x8924          /* set hardware address         */
#define SIOCGIFENCAP    0x8925          /* get/set encapsulations       */
#define SIOCSIFENCAP    0x8926
#define SIOCGIFHWADDR   0x8927          /* Get hardware address         */
#define SIOCGIFSLAVE    0x8929          /* Driver slaving support       */
#define SIOCSIFSLAVE    0x8930
#define SIOCADDMULTI    0x8931          /* Multicast address lists      */
#define SIOCDELMULTI    0x8932
#define SIOCGIFINDEX    0x8933          /* name -> if_index mapping     */
#define SIOGIFINDEX     SIOCGIFINDEX    /* misprint compatibility :-)   */
#define SIOCSIFPFLAGS   0x8934          /* set/get extended flags set   */
#define SIOCGIFPFLAGS   0x8935
#define SIOCDIFADDR     0x8936          /* delete PA address            */
#define SIOCSIFHWBROADCAST      0x8937  /* set hardware broadcast addr  */
#define SIOCGIFCOUNT    0x8938          /* get number of devices */

#define SIOCGIFBR       0x8940          /* Bridging support             */
#define SIOCSIFBR       0x8941          /* Set bridging options         */

#define SIOCGIFTXQLEN   0x8942          /* Get the tx queue length      */
#define SIOCSIFTXQLEN   0x8943          /* Set the tx queue length      */


/* ARP cache control calls. */
                    /*  0x8950 - 0x8952  * obsolete calls, don't re-use */
#define SIOCDARP        0x8953          /* delete ARP table entry       */
#define SIOCGARP        0x8954          /* get ARP table entry          */
#define SIOCSARP        0x8955          /* set ARP table entry          */

/* RARP cache control calls. */
#define SIOCDRARP       0x8960          /* delete RARP table entry      */
#define SIOCGRARP       0x8961          /* get RARP table entry         */
#define SIOCSRARP       0x8962          /* set RARP table entry         */

/* Driver configuration calls */

#define SIOCGIFMAP      0x8970          /* Get device parameters        */
#define SIOCSIFMAP      0x8971          /* Set device parameters        */

/* DLCI configuration calls */

#define SIOCADDDLCI     0x8980          /* Create new DLCI device       */
#define SIOCDELDLCI     0x8981          /* Delete DLCI device           */

/* Device private ioctl calls.  */

/* These 16 ioctls are available to devices via the do_ioctl() device
		  *    vector.  Each device should include this file and redefine these
		  *       names as their own. Because these are device dependent it is a good
		  *          idea _NOT_ to issue them to random objects and hope.  */

#define SIOCDEVPRIVATE          0x89F0  /* to 89FF */
 /*
			  *     These 16 ioctl calls are protocol private
			 *       */

#define SIOCPROTOPRIVATE 0x89E0 /* to 89EF */
#endif 
#ifndef SO_REUSEPORT
#define SO_REUSEPORT 15
#endif
#ifndef ICMPV6_FILTER
#define ICMPV6_FILTER                       1
#endif

int lwip_accept(int s, struct sockaddr *addr, socklen_t *addrlen);
int lwip_bind(int s, struct sockaddr *name, socklen_t namelen);
int lwip_shutdown(int s, int how);
int lwip_getpeername (int s, struct sockaddr *name, socklen_t *namelen);
int lwip_getsockname (int s, struct sockaddr *name, socklen_t *namelen);
int lwip_getsockopt (int s, int level, int optname, void *optval, socklen_t *optlen);
int lwip_setsockopt (int s, int level, int optname, const void *optval, socklen_t optlen);
int lwip_close(int s);
int lwip_connect(int s, struct sockaddr *name, socklen_t namelen);
int lwip_listen(int s, int backlog);
ssize_t lwip_recv(int s, void *mem, int len, unsigned int flags);
ssize_t lwip_read(int s, void *mem, int len);
ssize_t lwip_recvfrom(int s, void *mem, int len, unsigned int flags,
      struct sockaddr *from, socklen_t *fromlen);
ssize_t lwip_recvmsg(int fd, struct msghdr *msg, int flags);
ssize_t lwip_send(int s, void *dataptr, int size, unsigned int flags);
ssize_t lwip_sendto(int s, void *dataptr, int size, unsigned int flags,
    struct sockaddr *to, socklen_t tolen);
ssize_t lwip_sendmsg(int fd, struct msghdr *msg, int flags);
int lwip_msocket(struct stack *stack, int domain, int type, int protocol);
int lwip_socket(int domain, int type, int protocol);
ssize_t lwip_write(int s, void *dataptr, int size);
int lwip_select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset,
                struct timeval *timeout);
int lwip_ioctl(int s, unsigned long cmd, void *argp);
int lwip_fcntl64(int s, int cmd, long arg);
int lwip_fcntl(int s, int cmd, long arg);

/*
 * Taken from sys/uio.h and bits/uio.h
 */

/* Size of object which can be written atomically.
   This macro has different values in different kernel versions.  The
   latest versions of ther kernel use 1024 and this is good choice.  Since
   the C library implementation of readv/writev is able to emulate the
   functionality even if the currently running kernel does not support
   this large value the readv/writev call will not fail because of this.  */
#define UIO_MAXIOV      1024

#define size_t          u32_t

#if 0
/* Structure for scatter/gather I/O.  */
struct iovec {
	void *iov_base;     /* Pointer to data.  */
	int iov_len;     /* Length of data.  */

/* FIX: iov_len, and many other parameters should be size_t or socklen_t */

};
#endif

ssize_t lwip_readv(int s, struct iovec *vector, int count);
ssize_t lwip_writev(int s, struct iovec *vectorc, int count);


#if LWIP_COMPAT_SOCKETS
#define accept(a,b,c)         lwip_accept(a,b,c)
#define bind(a,b,c)           lwip_bind(a,b,c)
#define shutdown(a,b)         lwip_shutdown(a,b)
#define close(s)              lwip_close(s)
#define connect(a,b,c)        lwip_connect(a,b,c)
#define getsockname(a,b,c)    lwip_getsockname(a,b,c)
#define getpeername(a,b,c)    lwip_getpeername(a,b,c)
#define setsockopt(a,b,c,d,e) lwip_setsockopt(a,b,c,d,e)
#define getsockopt(a,b,c,d,e) lwip_getsockopt(a,b,c,d,e)
#define listen(a,b)           lwip_listen(a,b)
#define recv(a,b,c,d)         lwip_recv(a,b,c,d)
#define read(a,b,c)           lwip_read(a,b,c)
#define recvfrom(a,b,c,d,e,f) lwip_recvfrom(a,b,c,d,e,f)
#define send(a,b,c,d)         lwip_send(a,b,c,d)
#define sendto(a,b,c,d,e,f)   lwip_sendto(a,b,c,d,e,f)
#define socket(a,b,c)         lwip_socket(a,b,c)
#define write(a,b,c)          lwip_write(a,b,c)
#define select(a,b,c,d,e)     lwip_select(a,b,c,d,e)
#define ioctlsocket(a,b,c)    lwip_ioctl(a,b,c)

#define writev(a,b,c)         lwip_writev(a,b,c)
#define readv(a,b,c)          lwip_readv(a,b,c)

#endif /* LWIP_COMPAT_SOCKETS */

#endif /* __LWIP_SOCKETS_H__ */

