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
#include "lwip/opt.h"

#if LWIP_HAVE_LOOPIF

#include "lwip/def.h"

#include "netif/loopif.h"
#include "lwip/mem.h"

#if defined(LWIP_DEBUG) && defined(LWIP_TCPDUMP)
#include "netif/tcpdump.h"
#endif /* LWIP_DEBUG && LWIP_TCPDUMP */

#include "lwip/tcp.h"
#include "lwip/ip.h"

#if LWIP_NL
#include "lwip/arphdr.h"
#endif

#include "lwip/stack.h"
#if 0
static void
loopif_input( void * arg )
{
  //printf("loopif_input\n");
  struct netif *netif = (struct netif *)( ((void **)arg)[ 0 ] );
  struct pbuf *r = (struct pbuf *)( ((void **)arg)[ 1 ] );

  //printf("R %p NETIF %p\n",r,netif);
  mem_free( arg );
  netif->input( r, netif );
}
#endif

static err_t
loopif_output(struct netif *netif, struct pbuf *p, struct ip_addr *ipaddr)
{
  struct pbuf *q, *r;
  char *ptr;
  //void **arg;

#if defined(LWIP_DEBUG) && defined(LWIP_TCPDUMP)
  tcpdump(p);
#endif /* LWIP_DEBUG && LWIP_TCPDUMP */

  if (! (netif->flags & NETIF_FLAG_UP)) {
    return ERR_OK;
  }
		
  //printf("LOOPIFpbuf_alloc\n");
  r = pbuf_alloc(PBUF_RAW, p->tot_len, PBUF_RAM);
  //printf("LOOPIFpbuf_alloc done\n");
  if (r != NULL) {
    ptr = r->payload;
    
    for(q = p; q != NULL; q = q->next) {
      memcpy(ptr, q->payload, q->len);
      ptr += q->len;
    }
    netif->input( r, netif );

    /* arg = mem_malloc( sizeof( void *[2]));
	if( NULL == arg ) {
		return ERR_MEM; */
	/*}*/
	
	/*arg[0] = netif;
	arg[1] = r; */
	/**
	 * workaround (patch #1779) to try to prevent bug #2595:
	 * When connecting to "localhost" with the loopif interface,
	 * tcp_output doesn't get the opportunity to finnish sending the
	 * segment before tcp_process gets it, resulting in tcp_process
	 * referencing pcb->unacked-> which still is NULL.
	 * 
	 * TODO: Is there still a race condition here? Leon
	 */
	//printf("R %p NETIF %p\n",r,netif);
	//sys_timeout( 1, loopif_input, arg );
	
    return ERR_OK;    
  }

  return ERR_MEM;
}

err_t
loopif_init(struct netif *netif)
{
  struct stack *stack = netif->stack;
  
  netif->name[0] = 'l';
  netif->name[1] = 'o';
	netif->link_type = NETIF_LOOPIF;
	netif->num = netif_next_num(netif,NETIF_LOOPIF);
  netif->output = loopif_output;
  netif->flags |= NETIF_FLAG_UP | NETIF_FLAG_LOOPBACK;
#if LWIP_NL
  netif->type = ARPHRD_LOOPBACK;
#endif
  return ERR_OK;
}

#endif /* LWIP_HAVE_LOOPIF */







