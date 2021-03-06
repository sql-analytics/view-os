/*   This is part of LWIPv6
 *   
 *   Copyright 2004,2008,2011 Renzo Davoli University of Bologna - Italy
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
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
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

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>

#include "netif/etharp.h"

#include "lwip/debug.h"

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/ip.h"
#include "lwip/mem.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"

#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#define DEVTAP "/dev/net/tun"

/*-----------------------------------------------------------------------------------*/

#ifndef TUNIF_DEBUG
#define TUNIF_DEBUG                     DBG_OFF
#endif

#define IFNAME0 't'
#define IFNAME1 'n'

/*-----------------------------------------------------------------------------------*/

struct tunif {
  /* Add whatever per-interface state that is needed here. */
  int fd;
	struct netif_fddata *fddata;
};

/* Forward declarations. */
static void  tunif_input(struct netif_fddata *fddata, short revents);
static err_t tunif_output(struct netif *netif, struct pbuf *p,
			       struct ip_addr *ipaddr);

/*-----------------------------------------------------------------------------------*/
static int
low_level_init(struct netif *netif, char *ifname)
{
  struct tunif *tunif;

  tunif = netif->state;
  
  /* Obtain MAC address from network interface. */

  /* Do whatever else is needed to initialize interface. */
  
  tunif->fd = open(DEVTAP, O_RDWR);
  LWIP_DEBUGF(TUNIF_DEBUG, ("tunif_init: fd %d\n", tunif->fd));
  if (tunif->fd == -1) {
    perror("tunif_init");
   return ERR_IF;
  }
	{
		struct ifreq ifr;
		memset(&ifr, 0, sizeof(ifr));
		ifr.ifr_flags = IFF_TUN|IFF_NO_PI;
		strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
		if (ioctl(tunif->fd, TUNSETIFF, (void *) &ifr) < 0) {
			perror("tunif_init: DEVTUN ioctl TUNSETIFF");
			return ERR_IF;
		}
	}

	if ((tunif->fddata=netif_addfd(netif,
					tunif->fd, tunif_input, NULL, 0, POLLIN)) == NULL)
		return ERR_IF;
	else
		return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
/*
 * low_level_output():
 *
 * Should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 */
/*-----------------------------------------------------------------------------------*/

static err_t
low_level_output(struct tunif *tunif, struct pbuf *p)
{
  struct pbuf *q;
  char buf[1500];
  char *bufptr;
  
  /* initiate transfer(); */

  
  bufptr = &buf[0];
  
  for(q = p; q != NULL; q = q->next) {
    /* Send the data from the pbuf to the interface, one pbuf at a
       time. The size of the data in each pbuf is kept in the ->len
       variable. */    
    /* send data from(q->payload, q->len); */
    bcopy(q->payload, bufptr, q->len);
    bufptr += q->len;
  }

  /* signal that packet should be sent(); */
  if (write(tunif->fd, buf, p->tot_len) == -1) {
    perror("tunif: write");
  }
  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
/*
 * low_level_input():
 *
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 */
/*-----------------------------------------------------------------------------------*/
static struct pbuf *
low_level_input(struct tunif *tunif,u16_t ifflags)
{
  struct pbuf *p, *q;
  u16_t len;
  char buf[1500];
  char *bufptr;

  /* Obtain the size of the packet and put it into the "len"
     variable. */
  len = read(tunif->fd, buf, sizeof(buf));
	if (! (ifflags & NETIF_FLAG_UP)) {
		LWIP_DEBUGF(TUNIF_DEBUG, ("tunif_output: interface DOWN, discarded\n"));
		return NULL;
	} 

  /* We allocate a pbuf chain of pbufs from the pool. */
  p = pbuf_alloc(PBUF_LINK, len, PBUF_POOL);
  
  if (p != NULL) {
    /* We iterate over the pbuf chain until we have read the entire
       packet into the pbuf. */
    bufptr = &buf[0];
    for(q = p; q != NULL; q = q->next) {
      /* Read enough bytes to fill this pbuf in the chain. The
         available data in the pbuf is given by the q->len
         variable. */
      /* read data into(q->payload, q->len); */
      bcopy(bufptr, q->payload, q->len);
      bufptr += q->len;
    }
    /* acknowledge that packet has been read(); */
  } else {
    /* drop packet(); */
  }

  return p;  
}
/*-----------------------------------------------------------------------------------*/
/*
 * tunif_output():
 *
 * This function is called by the TCP/IP stack when an IP packet
 * should be sent. It calls the function called low_level_output() to
 * do the actuall transmission of the packet.
 *
 */
/*-----------------------------------------------------------------------------------*/
static err_t
tunif_output(struct netif *netif, struct pbuf *p,
		  struct ip_addr *ipaddr)
{
  struct tunif *tunif;

  tunif = netif->state;
	if (! (netif->flags & NETIF_FLAG_UP)) {
		LWIP_DEBUGF(TUNIF_DEBUG, ("tunif_output: interface DOWN, discarded\n"));
		return ERR_OK;
	} else
		return low_level_output(tunif, p);
}
/*-----------------------------------------------------------------------------------*/
/*
 * tunif_input():
 *
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface.
 *
 */
/*-----------------------------------------------------------------------------------*/
static void
tunif_input(struct netif_fddata *fddata, short revents)
{
	struct netif *netif = fddata->netif;
  struct tunif *tunif;
  struct pbuf *p;

  tunif = netif->state;
  
  p = low_level_input(tunif,netif->flags);

  if (p == NULL) {
    LWIP_DEBUGF(TUNIF_DEBUG, ("tunif_input: low_level_input returned NULL\n"));
    return;
  }

	netif->input(p, netif);
}

/* cleanup: garbage collection */
static err_t tunif_ctl(struct netif *netif, int request, void *arg)
{
	struct tunif *tunif = netif->state;

	if (tunif) {

		switch (request) {

			case NETIFCTL_CLEANUP:
				close(tunif->fd);

				mem_free(tunif);
		}
	}
	return ERR_OK;
}

/*-----------------------------------------------------------------------------------*/
/*
 * tunif_init():
 *
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 */
/*-----------------------------------------------------------------------------------*/
err_t
tunif_init(struct netif *netif)
{
  struct tunif *tunif;
	char *ifname;
    
  tunif = mem_malloc(sizeof(struct tunif));
  if (!tunif)
      return ERR_MEM;
	ifname = netif->state; /*state is temporarily used to store the if name */
  netif->state = tunif;
  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
	netif->link_type = NETIF_TUNIF;
	netif->num=netif_next_num(netif,NETIF_TUNIF);
  netif->output = tunif_output;
	netif->netifctl = tunif_ctl;
	netif->mtu = 1500;
	netif->hwaddr_len = 0;
#if LWIP_NL
	netif->type = ARPHRD_NONE;
#endif
  
	if (low_level_init(netif,ifname) < 0) {
		mem_free(tunif);
		return ERR_IF;
	}

  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
