/*   This is part of LWIPv6
 *   Developed for the Ale4NET project
 *   Application Level Environment for Networking
 *   
 *   Copyright 2004,2010,2011 Renzo Davoli University of Bologna - Italy
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

/* ip.c
 *
 * This is the code for the IP layer for IPv6.
 *
 */

#include "lwip/opt.h"

#include "lwip/debug.h"
#include "lwip/stats.h"
#include "arch/perf.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/netif.h"
#include "lwip/ip.h"
#include "lwip/ip_addr.h"
#include "lwip/ip_frag.h"
#include "lwip/icmp.h"
#include "lwip/raw.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"

#include "lwip/inet.h"


#if LWIP_DHCP
#include "lwip/dhcp.h"
#endif 

#if IPv6_AUTO_CONFIGURATION
#include "lwip/ip_autoconf.h"
#endif 

#if IPv6_ROUTER_ADVERTISEMENT
#include "lwip/ip_radv.h"
#endif 

#if LWIP_USERFILTER
#include "lwip/userfilter.h"
#endif

#if LWIP_NAT
#include "lwip/nat/nat.h"
#endif

//#ifndef IP_DEBUG
//#define IP_DEBUG DBG_ON
//#endif

#ifdef LWSLIRP
#define NOSLIRP ,NULL
#else
#define NOSLIRP 
#endif

/*--------------------------------------------------------------------------*/

/* IPv4 ID counter */
/* FIX: race condition with fragmentation code */

INLINE static int ip_process_exthdr(struct stack *stack, u8_t hdr, char *exthdr, u8_t hpos, struct pbuf **p, struct pseudo_iphdr *piphdr);

/*--------------------------------------------------------------------------*/
                     
/* ip_init:
 *
 * Initializes the IP layer.
 */
void
ip_init(struct stack *stack)
{
  ip_route_list_init(stack); 

#if IPv4_FRAGMENTATION || IPv6_FRAGMENTATION
  ip_frag_reass_init(stack);
#endif

#if IPv6_AUTO_CONFIGURATION
  ip_autoconf_init(stack);
#endif 

#if IPv6_ROUTER_ADVERTISEMENT
  ip_radv_init(stack);
#endif 

#if LWIP_USERFILTER
  /* init UserFilter's internal tables */
	if (stack->stack_flags & LWIP_STACK_FLAG_USERFILTER) {
		userfilter_init(stack);
#if LWIP_NAT
		if (stack->stack_flags & LWIP_STACK_FLAG_UF_NAT)
			nat_init(stack);
		else
			stack->stack_nat = NULL;
#endif
	} else {
		stack->stack_userfilter = NULL;
#if LWIP_NAT
		stack->stack_nat = NULL;
#endif
	}
#endif
}

void 
ip_shutdown(struct stack *stack)
{
#if LWIP_USERFILTER
#if LWIP_NAT
	if (stack->stack_nat != NULL)
		nat_shutdown(stack);
#endif
	if (stack->stack_userfilter != NULL)
		userfilter_shutdown(stack);
#endif

#if IPv6_AUTO_CONFIGURATION
  ip_autoconf_shutdown(stack);
#endif 

#if IPv4_FRAGMENTATION || IPv6_FRAGMENTATION
  ip_frag_reass_shutdown(stack);
#endif

  ip_route_list_shutdown(stack);
}


/*--------------------------------------------------------------------------*/

/* ip_input:
 *
 * This function is called by the network interface device driver when an IP packet is
 * received. The function does the basic checks of the IP header such as packet size
 * being at least larger than the header size etc. If the packet was not destined for
 * us, the packet is forwarded (using ip_forward). The IP checksum is always checked.
 *
 * Finally, the packet is sent to the upper layer protocol input function.
 */
static void
ip_inpacket(struct stack *stack, struct ip_addr_list *addr, struct pbuf *p, struct pseudo_iphdr *piphdr
#ifdef LWSLIRP
		    , struct netif *slirpif
#endif
				) 
{

#if LWIP_USERFILTER
  if (UF_HOOK(stack, UF_IP_LOCAL_IN, &p, addr->netif, NULL, UF_FREE_BUF) <= 0) {
    return;
  }
#endif

  /* Check for particular options/operations */
  if (piphdr->version == 4) {
    struct ip4_hdr *ip4hdr = (struct ip4_hdr *) p->payload;

	/* See if this is a fragment */
    if ((IPH4_OFFSET(ip4hdr) & htons(IP_OFFMASK | IP_MF)) != 0) 
    {
#if IPv4_FRAGMENTATION 
      struct ip_addr src4, dest4;

      LWIP_DEBUGF(IP_DEBUG, ("IPv4 packet is a fragment (id=0x%04x tot_len=%u len=%u MF=%u offset=%u), calling ip_reass()\n",ntohs(IPH4_ID(ip4hdr)), p->tot_len, ntohs(IPH4_LEN(ip4hdr)), !!(IPH4_OFFSET(ip4hdr) & htons(IP_MF)), (ntohs(IPH4_OFFSET(ip4hdr)) & IP_OFFMASK)*8));
      /* reassemble the packet*/
      p = ip4_reass(stack, p);
      /* packet not fully reassembled yet? */
      if (p == NULL) {
        LWIP_DEBUGF(IP_DEBUG,("%s: packet cached p=%p\n", __func__, p));
        return;
      }

      /* We reassembled the original IP header. Rebuild pseudo header! */
      if ( ip_build_piphdr(piphdr, p, &src4, &dest4) < 0) {
        LWIP_DEBUGF(IP_DEBUG, ("IP packet dropped due to bad version number\n"));
        ip_debug_print(IP_DEBUG, p);
        pbuf_free(p);
        IP_STATS_INC(ip.err);
        IP_STATS_INC(ip.drop);
        return;
      }
#else
      LWIP_DEBUGF(IP_DEBUG | 2, ("IP packet dropped since it was fragmented\n"));
      pbuf_free(p);
      IP_STATS_INC(ip.opterr);
      IP_STATS_INC(ip.drop);
      return;
#endif
    } 
  }
  else 
  if (piphdr->version == 6) {
    /* Process extension headers before upper-layers protocols */
    if (is_ipv6_exthdr(piphdr->proto)) {
      char *ehp = ((char *) p->payload) + piphdr->iphdrlen;
      if (ip_process_exthdr(stack, piphdr->proto, ehp, 0, &p, piphdr) < 0)
        /* an error occurred. Stop */
        return;
    }
  }

#if LWIP_RAW
  raw_input(p, addr, piphdr);
#endif /* LWIP_RAW */


#if LWIP_USERFILTER && LWIP_NAT
  /* Reset NAT+tracking information before sending to the upper layer */
  nat_pbuf_reset(p);
#endif

  switch (piphdr->proto + (piphdr->version << 8)) {

#if LWIP_UDP
    case IP_PROTO_UDP + (4 << 8):
    case IP_PROTO_UDP + (6 << 8):
      LWIP_DEBUGF(IP_DEBUG,("->UDP\n"));

      udp_input(p, addr, piphdr
#ifdef LWSLIRP
					, slirpif
#endif
					);
      break;
#endif
#if LWIP_TCP
    case IP_PROTO_TCP + (4 << 8):
    case IP_PROTO_TCP + (6 << 8):
      LWIP_DEBUGF(IP_DEBUG,("->TCP\n"));
      tcp_input(p, addr, piphdr
#ifdef LWSLIRP
					, slirpif
#endif
					);
      break;
#endif
    case IP_PROTO_ICMP + (6 << 8):
    case IP_PROTO_ICMP4 + (4 << 8):
      LWIP_DEBUGF(IP_DEBUG,("->ICMP\n"));
      icmp_input(stack, p, addr, piphdr);
      break;
    default:
      LWIP_DEBUGF(IP_DEBUG, ("Unsupported transport protocol %u\n", piphdr->proto));
      /* send ICMP destination protocol unreachable */
      icmp_dest_unreach(stack, p, ICMP_DUR_PROTO);
      /*discard*/  
      IP_STATS_INC(ip.proterr);
      IP_STATS_INC(ip.drop);
      pbuf_free(p);
  }
}

/*--------------------------------------------------------------------------*/


/* ip_forward:
 *
 * Forwards an IP packet. It finds an appropriate route for the packet, decrements
 * the TTL value of the packet, adjusts the checksum and outputs the packet on the
 * appropriate interface.
 */

INLINE static void
ip_forward(struct stack *stack, struct pbuf *p, struct ip_hdr *iphdr, struct netif *inif, 
           struct netif *netif, struct ip_addr *nexthop,  struct pseudo_iphdr *piphdr)
{
  struct ip4_hdr *ip4hdr;

  PERF_START;

#if LWIP_USERFILTER
  if (UF_HOOK(stack, UF_IP_FORWARD, &p, NULL, netif, UF_FREE_BUF) <= 0) {
    return;
  }
#endif

  /* 
   * Check TimeToLive (Ipv4) or Hop-Limit Field (Ipv6)
   */
  if (IPH_V(iphdr) == 4) {
    ip4hdr = (struct ip4_hdr *) iphdr;
    IPH4_TTL_SET(ip4hdr, IPH4_TTL(ip4hdr) - 1);
    if (IPH4_TTL(ip4hdr) <= 0) {
      LWIP_DEBUGF(IP_DEBUG, ("ip_forward: dropped packet! TTL <= 0 "));
      /* Don't send ICMP messages in response to ICMP messages */
      if (piphdr->proto != IP_PROTO_ICMP4) 
        icmp_time_exceeded(stack, p, ICMP_TE_TTL);
      pbuf_free(p);
      return;
    }

    /* Incrementally update the IP checksum. */
    if (IPH4_CHKSUM(ip4hdr) >= htons(0xffff - 0x100)) {
      IPH4_CHKSUM_SET(ip4hdr, IPH4_CHKSUM(ip4hdr) + htons(0x100) + 1);
    } else {
      IPH4_CHKSUM_SET(ip4hdr, IPH4_CHKSUM(ip4hdr) + htons(0x100));
    }
  }
  else if (IPH_V(iphdr) == 6) {
    /* Decrement TTL and send ICMP if ttl == 0. */
    IPH_HOPLIMIT_SET(iphdr, IPH_HOPLIMIT(iphdr) -1);
    if (IPH_HOPLIMIT(iphdr) <= 0) {
      LWIP_DEBUGF(IP_DEBUG, ("ip_forward: dropped packet! HOPLIMIT <= 0 "));
      /* Don't send ICMP messages in response to ICMP messages */
      if (IPH_NEXTHDR(iphdr) != IP_PROTO_ICMP)
        icmp_time_exceeded(stack, p, ICMP_TE_TTL);
      pbuf_free(p);
      return;
    }
  }

  LWIP_DEBUGF(IP_DEBUG, ("ip_forward: forwarding packet to "));
  ip_addr_debug_print(IP_DEBUG, piphdr->dest);
  LWIP_DEBUGF(IP_DEBUG, (" via %c%c%d\n",netif->name[0], netif->name[1], netif->num));


#if LWIP_USERFILTER
  /* pbuf_free() is called by Caller */
  if (UF_HOOK(stack, UF_IP_POST_ROUTING, &p, NULL, netif, UF_FREE_BUF) <= 0) {
    return;
  }
#endif

  /*
   * Check IP Fragmentation. Packet Size > Next Hop's MTU? 
   */
  if (p->tot_len > netif->mtu) {

    if (IPH_V(iphdr) == 4) {
#if IPv4_FRAGMENTATION 
      ip4hdr = (struct ip4_hdr *) iphdr;

      if (IPH4_OFFSET(ip4hdr) & htons(IP_MF)) {
        LWIP_DEBUGF(IP_DEBUG, ("ip_forward: IPv4 DF bit set. Don't fragment!"));
        icmp_dest_unreach(stack, p, ICMP_DUR_FRAG /*, netif->mtu*/);
        pbuf_free(p);
        return;
      }
      else {
        /* we can frag the packet */
        IP_STATS_INC(ip.fw);
        IP_STATS_INC(ip.xmit);

        ip4_frag(stack, p , netif, nexthop);
      }
#else
      LWIP_DEBUGF(IP_DEBUG, ("ip_forward: fragmentation on forwarded packets not implemented!"));
      pbuf_free(p);
      return;
#endif
    } 
    else if (IPH_V(iphdr) == 6) {
      /* IPv6 doesn't fragment forwarded packets  */
      icmp_packet_too_big(stack, p, netif->mtu);
      pbuf_free(p);
      return;
    }
  }

  IP_STATS_INC(ip.fw);
  IP_STATS_INC(ip.xmit);


	/* LWSLIRP TEST */
#ifdef LWSLIRP
	if (netif->link_type == NETIF_SLIRPIF) {
		struct ip_addr_list *addr;

		LWIP_DEBUGF(IP_DEBUG, ("ip_forward: slirp"));
		if (piphdr->version == 6)
			/* in ipv6 the length field conatins only the legnth of the payload
			 * (the length of the ipv6 haeder is costant) */
			pbuf_realloc(p, IP_HLEN + ntohs(iphdr->len));
		else
			/* in ipv4 the legnth filed contains also the  length of the header
			 * (the length of the ipv4 header isn't costant) */
			pbuf_realloc(p, ntohs(IPH4_LEN(ip4hdr)));
		addr = ip_addr_list_alloc(stack);
		if(addr == NULL) {
			LWIP_DEBUGF(IP_DEBUG, ("ip_forward: ip_addr_list_alloc() failed, I abort\n"));
			/* There aren't no more ip_addr_list avaible, so I abort.*/
			abort();
		}
		addr->next = NULL;
		/* I control if the destination address is a multicast-solicited address,
		 * in this case I find the right unicast address and I copy it in addr->ipaddr.
		 * Otherwise the dest address is copied in addr->ipaddr */
		if(piphdr->version == 6 &&
				ip_addr_is_addr_solicited(piphdr->dest)) {
			struct ip_addr *unicast_addr;

			LWIP_DEBUGF(IP_DEBUG, ("ip_input: destination address("));
			ip_addr_debug_print(IP_DEBUG, piphdr->dest);
			LWIP_DEBUGF(IP_DEBUG, (") is a solicited IPv6 address\n"));

			unicast_addr = ip_addr_find_unicast_from_solicited(inif->addrs, piphdr->dest);

			/* If unicast_addr is NULL, the pachet is not for us */
			if(unicast_addr == NULL) {
				LWIP_DEBUGF(IP_DEBUG, ("ip_input: multicast packet not for us\n"));
				pbuf_free(p);
				ip_addr_list_free(stack,addr);
				return;
			}
			LWIP_DEBUGF(IP_DEBUG, ("ip_input: after ip_addr_find_unicast_from_solicited(), unicast_addr = "));
			ip_addr_debug_print(IP_DEBUG, unicast_addr);
			LWIP_DEBUGF(IP_DEBUG, ("\n"));

			memcpy(&addr->ipaddr, unicast_addr, sizeof(struct ip_addr));
		} else {
			memcpy(&addr->ipaddr, piphdr->dest, sizeof(struct ip_addr));
		}
		/* addr->netmask; unused? */
		addr->netif = inif;
		/* addr->flags; unused?*/

		/* I test if is not a solicited dest address and
		 * the packet is a ICMP packet. If it so, I discard
		 * it, because it's not for us, and I return */
		if(!ip_addr_is_addr_solicited(piphdr->dest) &&
				(piphdr->proto == IP_PROTO_ICMP4 || piphdr->proto == IP_PROTO_ICMP)) {
			LWIP_DEBUGF(IP_DEBUG, ("ip_input: ICMP packet, I discard it.\n"));

			pbuf_free(p);
			ip_addr_list_free(stack,addr);
			return;
		}
		/* Then I can pass the packet to an higher level of the stack */
		ip_inpacket(stack,addr, p, piphdr, netif);
		/* I free "addr"*/
		ip_addr_list_free(stack,addr);
		return;
	} else 
#endif
	{
		PERF_STOP("ip_forward");

		netif->output(netif, p, nexthop);

		pbuf_free(p);
	}
}

/*--------------------------------------------------------------------------*/

void
ip_input(struct pbuf *p, struct netif *inp) 
{
  struct ip_hdr *iphdr;
  struct ip4_hdr *ip4hdr;
  struct netif *netif;
  struct ip_addr_list *addrel;
#if IP_FORWARD
  struct ip_addr *nexthop;
  int fwflags;
#endif
  struct pseudo_iphdr piphdr;
  struct ip_addr src4,dest4;
  
  /* Get stack id from network interface */
  struct stack *stack = inp->stack;

  PERF_START;

  LWIP_DEBUGF(IP_DEBUG,("%s: new IP packet\n", __func__));
  ip_debug_print(IP_DEBUG, p);
  IP_STATS_INC(ip.recv);

  /* identify the IP header */
  iphdr = p->payload;
  ip4hdr = p->payload;

  /* Create a pseudo header used by the stack */
  if ( ip_build_piphdr(&piphdr, p, &src4, &dest4) < 0) {
    LWIP_DEBUGF(IP_DEBUG, ("IP packet dropped due to bad version number\n"));
    ip_debug_print(IP_DEBUG, p);
    pbuf_free(p);
    IP_STATS_INC(ip.err);
    IP_STATS_INC(ip.drop);
    return;
  }
#if IPv4_CHECK_CHECKSUM
  if (IPH_V(iphdr) == 4) {
    /* Only IPv4 has checksum field */
    u16_t sum = inet_chksum(ip4hdr, piphdr.iphdrlen);
    if (sum != 0) {
      pbuf_free(p);
      LWIP_DEBUGF(IP_DEBUG | 2, ("Checksum (0x%x, len=%d) failed, IP packet dropped.\n", sum, piphdr.iphdrlen));
      IP_STATS_INC(ip.chkerr);
      IP_STATS_INC(ip.drop);
      return;
    }
  }
#endif

#if LWIP_USERFILTER
  if (UF_HOOK(stack, UF_IP_PRE_ROUTING, &p, inp, NULL, UF_FREE_BUF) <= 0) {
    return;
  }
  /* NATed packets need a new pseudo header used by the stack */
  if ( ip_build_piphdr(&piphdr, p, &src4, &dest4) < 0) {
    LWIP_DEBUGF(IP_DEBUG, ("IP packet dropped due to bad version number\n"));
    ip_debug_print(IP_DEBUG, p);
    pbuf_free(p);
    IP_STATS_INC(ip.err);
    IP_STATS_INC(ip.drop);
    return;
  }
#endif       

  /* Find packet destination */
  if ((addrel = ip_addr_list_deliveryfind(inp->addrs, piphdr.dest, piphdr.src)) != NULL) 
  { 
    /* local address */
    if (piphdr.version == 6)
      pbuf_realloc(p, IP_HLEN + ntohs(iphdr->len)); 
    else
      pbuf_realloc(p, ntohs(IPH4_LEN(ip4hdr)));

    ip_inpacket(stack, addrel, p, &piphdr NOSLIRP);
		goto ip_input_end;
  }

  /* FIX: handle IPv6 Multicast in this way? */
  if (ip_addr_ismulticast(piphdr.dest)) {
    struct ip_addr_list tmpaddr;

    LWIP_DEBUGF(IP_DEBUG | 2, ("ip_input: multicast!\n"));

    tmpaddr.netif = inp;
    tmpaddr.flags = 0;
    IP6_ADDR_LINKSCOPE(&tmpaddr.ipaddr, inp->hwaddr);

    ip_inpacket(stack, &tmpaddr, p, &piphdr NOSLIRP);
	  goto ip_input_end;
  }

#if LWIP_DHCP
  /* Pass DHCP messages regardless of destination address. DHCP traffic is addressed
   * using link layer addressing (such as Ethernet MAC) so we must not filter on IP.
   * According to RFC 1542 section 3.1.1, referred by RFC 2131).
   */
  if (piphdr.version == 4 && IPH4_PROTO(ip4hdr) == IP_PROTO_UDP) {

    struct udp_hdr *udphdr = (struct udp_hdr *)((u8_t *)ip4hdr + piphdr.iphdrlen); 

    /* remote port is DHCP server? */
    LWIP_DEBUGF(IP_DEBUG, ("ip_input: UDP packet to DHCP client port %u\n", ntohs(udphdr->dest)));
    if (ntohs(udphdr->dest) == DHCP_CLIENT_PORT) {
      LWIP_DEBUGF(IP_DEBUG, ("ip_input: DHCP packet accepted.\n")); 
      struct ip_addr_list tmpaddr;

          memset(&tmpaddr, 0, sizeof(struct ip_addr_list));
      tmpaddr.netif = inp;
      tmpaddr.flags = 0;

      ip_inpacket(stack, &tmpaddr, p, &piphdr NOSLIRP);
      goto ip_input_end;
    }
  }
#endif /* LWIP_DHCP */

#if IP_FORWARD
  if ((stack->stack_flags & LWIP_STACK_FLAG_FORWARDING) && ip_route_findpath(stack, piphdr.dest, &nexthop, &netif, &fwflags) == ERR_OK && netif != inp)
  { 
    /* forwarding */
    ip_forward(stack, p, iphdr, inp, netif, nexthop, &piphdr);
    goto ip_input_end;
  }
#endif

	/* LWSLIRP TEST */
  LWIP_DEBUGF(IP_DEBUG | 2, ("ip_input: unable to route IP packet. Droped\n"));
  pbuf_free(p);

ip_input_end:
  PERF_STOP("ip_input");
}


/*--------------------------------------------------------------------------*/


/* ip_output_if:
 *
 * Sends an IP packet on a network interface. This function constructs the IP header
 * and calculates the IP header checksum. If the source IP address is NULL,
 * the IP address of the outgoing network interface is filled in as source address.
 */
err_t
ip_output_if (struct stack *stack, struct pbuf *p, struct ip_addr *src, struct ip_addr *dest,
              u8_t ttl, u8_t tos, u8_t proto, 
              struct netif *netif, struct ip_addr *nexthop, int flags)
{
  err_t  ret = ERR_OK; /* default return value */
  struct ip_hdr *iphdr;
  struct ip4_hdr *ip4hdr;
  u8_t version;
	struct ip_addr addrfromhdr;
#if LWIP_USERFILTER
  struct pbuf *caller_p;
#endif

	/*fprintf(stderr, "ip_output_if %p\n", src);*/
	if (src == NULL) {
		fprintf(stderr, "*^*^*^ ip_output_if NULL!\n");
		return ERR_BUF;
	}

  PERF_START;

  if (ip_addr_isany(src)) 
  	version = ip_addr_is_v4comp(dest) ? 4 : 6;
  else 
  	version = (ip_addr_is_v4comp(src) && ip_addr_is_v4comp(dest)) ? 4 : 6;

  /* Get size for the IP header */
  if (dest != IP_LWHDRINCL && pbuf_header(p, version==6?IP_HLEN:IP4_HLEN)) {
    LWIP_DEBUGF(IP_DEBUG, ("ip_output: not enough room for IP header in pbuf\n"));
    IP_STATS_INC(ip.err);
    return ERR_BUF;
  }

  /* Create IP header */
  if(version == 6) {
    iphdr = p->payload;
    if (dest != IP_LWHDRINCL) {
      iphdr->_v_cl_fl = 0;
      IPH_NEXT_HOP_SET(iphdr, proto, ttl);
      iphdr->len = htons(p->tot_len - IP_HLEN);
      IPH_V_SET(iphdr, 6);
      ip_addr_set(&(iphdr->dest), dest);
      ip_addr_set(&(iphdr->src), src);
    } else {
      dest = &(iphdr->dest);
    }
  } 
  else { /* IPv4 */
    ip4hdr = p->payload;
    if (dest != IP_LWHDRINCL) {
      IPH4_TTL_SET(ip4hdr, ttl);
      IPH4_PROTO_SET(ip4hdr, proto);
      ip64_addr_set(&(ip4hdr->dest), dest);
      IPH4_VHLTOS_SET(ip4hdr, 4, IP4_HLEN / 4, tos);
      IPH4_LEN_SET(ip4hdr, htons(p->tot_len));
      IPH4_OFFSET_SET(ip4hdr, htons(IP_DF));
      IPH4_ID_SET(ip4hdr, htons(stack->ip_id));
      ++stack->ip_id;
      ip64_addr_set(&(ip4hdr->src), src);
      IPH4_CHKSUM_SET(ip4hdr, 0);
      IPH4_CHKSUM_SET(ip4hdr, inet_chksum(ip4hdr, IP4_HLEN));
    } else {
      IPH4_LEN_SET(ip4hdr, htons(p->tot_len));
			/* from raw(7) man page: but it is not true! Linux kernel
			 * does not clean src and ID */
      /* ip64_addr_set(&(ip4hdr->src), 0); 
      IPH4_ID_SET(ip4hdr, 0); */
      IPH4_CHKSUM_SET(ip4hdr, 0);
      IPH4_CHKSUM_SET(ip4hdr, inet_chksum(ip4hdr, IP4_HLEN));
			IP64_CONV(&addrfromhdr,&(ip4hdr->dest));
      dest = &addrfromhdr;
    }
  }

#if LWIP_USERFILTER
  /* The Caller of ip_output_if() will call pbuf_free() on buffer pointed by 'p' 
     after return.
     Hooks could call pbuf_free() on 'p' too, so we need to increase reference.
     IF hooks replace buffer pointed by 'p' with an other, we have to free the new
     buffer before return. */
  caller_p = p;
	/* this allocation must be undone in end_ip_output_if when the buffer does not change */
  pbuf_ref(caller_p);
#endif

#if LWIP_USERFILTER
  /* FIX: LOCAL_OUT after routing decisions? It this the right place */
  if (UF_HOOK(stack, UF_IP_LOCAL_OUT, &p, NULL, netif, UF_DONTFREE_BUF) <= 0) {
    goto end_ip_output_if;
  }
#endif

#ifdef IP_DEBUG
  LWIP_DEBUGF(IP_DEBUG, ("\nip_output_if: %c%c (len %u)\n", netif->name[0], netif->name[1], p->tot_len));
  ip_debug_print(IP_DEBUG, p);
#endif /* IP_DEBUG */


  IP_STATS_INC(ip.xmit);

  PERF_STOP("ip_output_if");

  /* The packet is for us? */
  if (ip_addr_cmp(src,dest)) {
    struct pbuf *r;

    LWIP_DEBUGF(IP_DEBUG, ("\t PACKET FOR US\n"));

    if (! (netif->flags & NETIF_FLAG_UP)) {
      goto end_ip_output_if;
    }

    /* The caller will destroy 'p' after return. We need to create a clone! */
    r = pbuf_clone(PBUF_RAW, p, PBUF_RAM);
    if (r != NULL) {

#if LWIP_USERFILTER
      if (UF_HOOK(stack, UF_IP_POST_ROUTING, &r, NULL, netif, UF_DONTFREE_BUF) <= 0) {
        goto end_ip_output_if;
      }
#if LWIP_NAT
      /* Reset NAT+tracking information before sending packet back to us */
      nat_pbuf_reset(r);
#endif

#endif
      netif->input( r, netif );
	  //ip_input(r, netif);
    }

    goto end_ip_output_if;
  }
  /* packet for remote host */ 
  else {
    LWIP_DEBUGF(IP_DEBUG, ("SENDING OUT %c%c%d\n", netif->name[0],netif->name[1],netif->num));
	
#if LWIP_USERFILTER
    if (UF_HOOK(stack, UF_IP_POST_ROUTING, &p, NULL, netif, UF_DONTFREE_BUF) <= 0) {
      goto end_ip_output_if;
    }
#endif

    /* Handle fragmentation */
    if (netif->mtu && (p->tot_len > netif->mtu)) {
      LWIP_DEBUGF(IP_DEBUG, ("ip_output_if: packet need fragmentation (len=%d, mtu=%d)\n",p->tot_len,netif->mtu));
#if IPv4_FRAGMENTATION 
      if (version == 4) {
        ret = ip4_frag(stack, p , netif, nexthop);
        goto end_ip_output_if;
      }
#endif
#if IPv6_FRAGMENTATION 
      if (version == 6) {
        ret = ip6_frag(stack, p , netif, nexthop);
        goto end_ip_output_if;
      }
#endif
      LWIP_DEBUGF(IP_DEBUG, ("ip_output_if: fragmentation not supported. Dropped!\n"));
      /* FIX: error code? */
    }
    else {
      LWIP_DEBUGF(IP_DEBUG, ("ip_output_if: nettif->output()\n"));
      ret = netif->output(netif, p, nexthop);
    }
  }


end_ip_output_if:

#if LWIP_USERFILTER
  if (caller_p != p) {
    /* Somewhere, inside a hook, we've changed packet's buffer. In this case
       we have to free it because Caller will call pbuf_free() only on 
       buffer pointed by caller_p */
    pbuf_free(p);
	} else {
		/* the buffer did not change so we must undo the pbuf_ref above */
		pbuf_free(caller_p);
	}
#endif

  return ret;
}


/*--------------------------------------------------------------------------*/


/* ip_output:
 *
 * Simple interface to ip_output_if. It finds the outgoing network interface and
 * calls upon ip_output_if to do the actual work.
 */
err_t
ip_output(struct stack *stack, struct pbuf *p, struct ip_addr *src, struct ip_addr *dest, 
          u8_t ttl, u8_t tos, u8_t proto)
{                      
  struct netif *netif;
  struct ip_addr *nexthop;
  int flags;

  LWIP_DEBUGF(IP_DEBUG, ("%s: start\n", __func__));

  LWIP_DEBUGF(IP_DEBUG, ("src="));
  ip_addr_debug_print(IP_DEBUG, src);
  LWIP_DEBUGF(IP_DEBUG, ("  dest="));
  ip_addr_debug_print(IP_DEBUG, dest);
  LWIP_DEBUGF(IP_DEBUG, ("  ttl=%d", ttl));
  LWIP_DEBUGF(IP_DEBUG, ("  proto=%d\n", proto));

  if (ip_route_findpath(stack, dest, &nexthop, &netif, &flags) != ERR_OK) {
    LWIP_DEBUGF(IP_DEBUG, ("ip_output: No route to XXX \n" ));
    IP_STATS_INC(ip.rterr);
    return ERR_RTE;
  }
  else {
		if (src==NULL) {
			struct ip_addr_list *el;
			if ((el=ip_route_select_source_ip(netif, dest, nexthop)) == NULL)
				return ERR_RTE;
			src = &(el->ipaddr);
		}
    return ip_output_if (stack, p, src, dest, ttl, tos, proto, netif, nexthop, flags);
  }
}


/*--------------------------------------------------------------------------*/


/* This function inform the IP Layer that interface's properties have been
   changed. */
void 
ip_notify(struct netif *netif, u32_t type)
{
  struct stack *stack = netif->stack;

  switch(type) {

    case NETIF_CHANGE_UP:
      LWIP_DEBUGF(IP_DEBUG, ("%s: netif %c%c%d now UP!\n", __func__,	
        netif->name[0], netif->name[1], netif->num));

#if LWIP_DHCP
			if (netif->flags & NETIF_FLAG_DHCP)
				dhcp_start(netif);
#endif

#if IPv6_AUTO_CONFIGURATION
			if (netif->flags & NETIF_FLAG_AUTOCONF)
				ip_autoconf_start(netif);
#endif

#if IPv6_ROUTER_ADVERTISEMENT
			if (netif->flags & NETIF_FLAG_RADV)
				ip_radv_start(netif);
#endif
      break;

    case NETIF_CHANGE_DOWN:
      LWIP_DEBUGF(IP_DEBUG, ("%s: netif %c%c%d now DOWN!\n", __func__, 
        netif->name[0], netif->name[1], netif->num));

#if LWIP_DHCP
			if (netif->flags & NETIF_FLAG_DHCP) {
				dhcp_release(netif);
				dhcp_stop(netif);
			}
#endif

#if IPv6_ROUTER_ADVERTISEMENT
			if (netif->flags & NETIF_FLAG_RADV)
				ip_radv_stop(netif);
#endif

#if IPv6_AUTO_CONFIGURATION
			if (netif->flags & NETIF_FLAG_AUTOCONF)
				ip_autoconf_stop(netif);
#endif
      break;

    case NETIF_CHANGE_MTU:
      LWIP_DEBUGF(IP_DEBUG, ("%s: netif %c%c%d changed MTU, now %d!\n", __func__,
        netif->name[0], netif->name[1], netif->num, netif->mtu));
#if IPv6_ROUTER_ADVERTISEMENT
        /* todo? */
#endif
        break;

    default:
        LWIP_DEBUGF(IP_DEBUG, ("%s: unknown change *** BUG ***\n", __func__));
        break;
  }
}


/*--------------------------------------------------------------------------*/

#ifdef IP_DEBUG

INLINE static void 
ip_debug_print_transport(u8_t proto, void *hdr)
{
  struct icmp_echo_hdr *icmph = NULL;
  struct tcp_hdr       *tcphdr  = NULL;
  struct udp_hdr       *udphdr  = NULL;

  switch (proto)
  {
    case IP_PROTO_TCP:
      tcphdr = hdr;
      LWIP_DEBUGF(IP_DEBUG, ("TCP [%d,%d]", ntohs(tcphdr->src), ntohs(tcphdr->dest))); 
    break;
    case IP_PROTO_UDP:
      udphdr = hdr;
      LWIP_DEBUGF(IP_DEBUG, ("UDP [%d,%d]", ntohs(udphdr->src), ntohs(udphdr->dest))); 
    break;
    case IP_PROTO_ICMP4:
      icmph = hdr;
      LWIP_DEBUGF(IP_DEBUG, ("Icmp4 id=%d type=%d code=%d", 
      ntohs(icmph->id), 
        (char)ICMPH_TYPE(icmph), 
        (char)icmph->icode)); 
    break;
    case IP_PROTO_ICMP:
      icmph = hdr;
      LWIP_DEBUGF(IP_DEBUG, ("Icmp6 id=%d type=%d code=%d", 
      ntohs(icmph->id), 
        (unsigned char)ICMPH_TYPE(icmph), 
        (unsigned char)icmph->icode)); 
    break;
    default:
      LWIP_DEBUGF(IP_DEBUG, ("%s: strange protocol", __func__ ));
      break;
  }
  LWIP_DEBUGF(IP_DEBUG, ("\n"));
}

INLINE static void
ip4_debug_print(struct pbuf *p)
{
  struct ip_addr tempsrc, tempdest;
  struct ip4_hdr *iphdr = p->payload;
  u8_t *payload;

  payload = (u8_t *)iphdr + IP4_HLEN;
  LWIP_DEBUGF(IP_DEBUG, ("IPv4 Packet:  "));
  IP64_CONV(&tempsrc, &(iphdr->src));
  IP64_CONV(&tempdest, &(iphdr->dest));
  LWIP_DEBUGF(IP_DEBUG, ("src="));
  ip_addr_debug_print(IP_DEBUG, &tempsrc);
  LWIP_DEBUGF(IP_DEBUG, ("  dest="));
  ip_addr_debug_print(IP_DEBUG, &tempdest);
  LWIP_DEBUGF(IP_DEBUG, ("  proto=%u  ttl=%u  chksum=0x%04x",
                        IPH4_PROTO(iphdr),
                        IPH4_TTL(iphdr),
                        ntohs(IPH4_CHKSUM(iphdr))));
  LWIP_DEBUGF(IP_DEBUG, ("  id=%u  flags=%u%u%u  offset=%u\n",
                        ntohs(IPH4_ID(iphdr)),
                        ntohs(IPH4_OFFSET(iphdr)) >> 15 & 1,
                        ntohs(IPH4_OFFSET(iphdr)) >> 14 & 1,
                        ntohs(IPH4_OFFSET(iphdr)) >> 13 & 1,
                        ntohs(IPH4_OFFSET(iphdr)) & IP_OFFMASK));

  ip_debug_print_transport(IPH4_PROTO(iphdr), payload);
}

INLINE static void
ip6_debug_print(int how, struct pbuf *p)
{
  struct ip_hdr *iphdr = p->payload;
  char *payload;

  payload = (char *)iphdr + IP_HLEN;
  LWIP_DEBUGF(how, ("IPv6 packet:\n"));
  LWIP_DEBUGF(how, ("src="));
  ip_addr_debug_print(how, &iphdr->src);
  LWIP_DEBUGF(how, ("  dest="));
  ip_addr_debug_print(how, &iphdr->dest);
  LWIP_DEBUGF(how, ("  nexthdr=%2u  hoplim=%2u\n", IPH_NEXTHDR(iphdr), IPH_HOPLIMIT(iphdr)));

  ip_debug_print_transport( IPH_NEXTHDR(iphdr), payload);
}

void
ip_debug_print(int how, struct pbuf *p)
{
  struct ip_hdr *iphdr = p->payload;
  if (IPH_V(iphdr) == 4) 
    ip4_debug_print(p);
  else                   
    ip6_debug_print(how,p);
}

#endif /* IP_DEBUG */


/*--------------------------------------------------------------------------*/

/**
 * Build the pseudo header.
 * It takes in input the pseudo header to fill and, pbuf
 * with the IP packet from where take the information, and
 * the two IPv4 addresses fill with the IPv4 addresses if
 * the packet in IPv4.
 * It returns 1 if the pseudo header is fill without error,
 * 0 if the the packet contains a bad IP version number.
 */
int 
ip_build_piphdr(struct pseudo_iphdr *piphdr, struct pbuf *p, 
                struct ip_addr *src4, struct ip_addr *dest4) 
{
  struct ip_hdr *iphdr;
  struct ip4_hdr *ip4hdr;

  iphdr = p->payload;

  /* I identify the IP header */
  piphdr->version=IPH_V(iphdr);
  if (piphdr->version == 6) {
    /* it's a ipv6 packet, I fill the pseudo header */
    piphdr->proto    = IPH_NEXTHDR(iphdr);
    piphdr->iphdrlen = IP_HLEN;
    piphdr->src      = &(iphdr->src);
    piphdr->dest     = &(iphdr->dest);
    return 1; /* All ok*/
  }
  else if (piphdr->version == 4) {
    /* it's a ipv4 packet, I fill the pseudo header */
    ip4hdr = p->payload;
    piphdr->proto    = IPH4_PROTO(ip4hdr);
    piphdr->iphdrlen = IPH4_HL(ip4hdr) * 4;
    /* coversion of the source and destination addresses
    * from ipv4 to "ipv4-mapped ipv6 address".
    * This kind of address is used in device that understand
    * only ipv4. */
    IP64_CONV(src4,&(ip4hdr->src));
    IP64_CONV(dest4,&(ip4hdr->dest));
    piphdr->src = src4;
    piphdr->dest = dest4;

    return 1; /* All ok*/
  } else {
    /* bad version number */
    return -1; /* error */
  }
}

/*
 * Process Extension headers of IPv6.
 */
INLINE static int 
ip_process_exthdr(struct stack *stack, u8_t hdr, char *exthdr, u8_t hpos, 
                  struct pbuf **p, struct pseudo_iphdr *piphdr)
{
  struct ip_exthdr *prevhdr; /* previous ext header */
  u8_t loop = 1;
  int r = -1;

  LWIP_DEBUGF(IP_DEBUG, ("%s: Start processing extension headers.\n", __func__));

  /* It loops while there are extension headers */
  prevhdr = NULL;
  do {

    /* FIX: check p->tot_len and "hdr" position to avoid bufferoverflows */

    switch (hdr) {
      case IP6_NEXTHDR_HOP:
      case IP6_NEXTHDR_DEST:
      case IP6_NEXTHDR_ROUTING:
        /* Drop packet */
        LWIP_DEBUGF(IP_DEBUG, ("Extension header %d not yet supported\n", hdr));
        pbuf_free(*p);
        *p=NULL;
        loop = 0;
        break;         

      case IP6_NEXTHDR_FRAGMENT: 
#if IPv6_FRAGMENTATION 
      {
        struct ip6_fraghdr *fhdr = (struct ip6_fraghdr *) exthdr;
        LWIP_DEBUGF(IP_DEBUG, ("Fragment Header\n"));

        *p = ip6_reass(stack, *p, fhdr, prevhdr); 
        if (*p == NULL) {
          /* Don't free 'p'. Fragmentation code has "stolen" the packet */
          LWIP_DEBUGF(IP_DEBUG,("\tpacket cached p=%p\n", *p));
        } else {
          LWIP_DEBUGF(IP_DEBUG,("\tNew pseudo header p=%p\n", *p));
          ip_build_piphdr(piphdr, *p, piphdr->src, piphdr->dest);
          r = 1;
        }

        /* Go to the next header */
        hdr    = fhdr->nexthdr;
        exthdr = exthdr + sizeof(struct ip6_fraghdr);
        break;
      }
#endif
      case IP6_NEXTHDR_ESP:
      case IP6_NEXTHDR_AUTH:
      case IP6_NEXTHDR_NONE: /* NOTE: this is useful only on forwarded packets? */
        /* Drop packet */
        LWIP_DEBUGF(IP_DEBUG, ("Extension header %d not yet supported\n", hdr));
        pbuf_free(*p);
        *p=NULL;
        loop = 0;
        break;

      default:
        /* We found the first non-extention header. We can exit */
        LWIP_DEBUGF(IP_DEBUG, ("Protocol %d is not an extension headers.\n", hdr));
        loop = 0;
        break;
    }
  }
  while (loop);

  LWIP_DEBUGF(IP_DEBUG, ("ip_process_exthdr: Stop\n"));

  return r;
}

