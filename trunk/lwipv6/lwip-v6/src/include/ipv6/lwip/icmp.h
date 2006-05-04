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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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
#ifndef __LWIP_ICMP_H__
#define __LWIP_ICMP_H__

#include "lwip/arch.h"

#include "lwip/opt.h"
#include "lwip/pbuf.h"

#include "lwip/netif.h"

#define ICMP4_ER      0    /* echo reply */
#define ICMP4_DUR     3    /* destination unreachable */
#define ICMP4_SQ      4    /* source quench */
#define ICMP4_RD      5    /* redirect */
#define ICMP4_ECHO    8    /* echo */
#define ICMP4_TE     11    /* time exceeded */
#define ICMP4_PP     12    /* parameter problem */
#define ICMP4_TS     13    /* timestamp */
#define ICMP4_TSR    14    /* timestamp reply */
#define ICMP4_IRQ    15    /* information request */
#define ICMP4_IR     16    /* information reply */

#define ICMP6_DUR     1
#define ICMP6_PTB     2    /* Packet Too Big */
#define ICMP6_TE      3
#define ICMP6_ECHO  128    /* echo */
#define ICMP6_ER    129    /* echo reply */
#define ICMP6_RS    133    /* router solicitation */
#define ICMP6_RA    134    /* router advertisement */
#define ICMP6_NS    135    /* neighbor solicitation */
#define ICMP6_NA    136    /* neighbor advertisement */

enum icmp_dur_type {
  ICMP_DUR_NET   = 0,  /* net unreachable */
  ICMP_DUR_HOST  = 1,  /* host unreachable */
  ICMP_DUR_PROTO = 2,  /* protocol unreachable */
  ICMP_DUR_PORT  = 3,  /* port unreachable */
  ICMP_DUR_FRAG  = 4,  /* fragmentation needed and DF set */
  ICMP_DUR_SR    = 5   /* source route failed */
};

enum icmp_te_type {
  ICMP_TE_TTL  = 0,    /* time to live exceeded in transit */
  ICMP_TE_FRAG = 1     /* fragment reassembly time exceeded */
};

void icmp_input(struct pbuf *p, struct ip_addr_list *inad, struct pseudo_iphdr *piphdr);

void icmp_send_dad(struct ip_addr_list *targetip, struct netif *srcnetif);


void icmp_neighbor_solicitation(struct ip_addr *ipaddr, struct ip_addr_list *inad);
void icmp_router_solicitation(struct ip_addr *ipaddr, struct ip_addr_list *inad);

void icmp_dest_unreach(struct pbuf *p, enum icmp_dur_type t);
void icmp_time_exceeded(struct pbuf *p, enum icmp_te_type t);

void icmp_packet_too_big(struct pbuf *p, u16_t mtu);

void icmp4_dest_unreach(struct pbuf *p, enum icmp_dur_type t, u16_t nextmtu );
void icmp4_time_exceeded(struct pbuf *p, enum icmp_te_type t);

/*
 * ICMP Headers used for IPv4 and IPv6
 */

#define ICMPH_TYPE_SET(hdr,typ) (hdr)->type=(typ)

#define ICMPH_TYPE(hdr)   ((hdr)->type)
#define ICMPH_CODE(hdr)   ((hdr)->icode)
#define ICMPH_CHKSUM(hdr) ((hdr)->chksum)

/* Echo Request, Echo Reply */
struct icmp_echo_hdr {	
  u8_t type;
  u8_t icode;
  u16_t chksum;
  u16_t id;
  u16_t seqno;
};

/* Destination Unreachable */
struct icmp_dur_hdr { 	
  u8_t type;
  u8_t icode;
  u16_t chksum;
  u16_t unused;
  u16_t nextmtu;    /* this is used only in ICMPv4 packets */
};

/* Time Exceeded */
struct icmp_te_hdr {	
  u8_t type;
  u8_t icode;
  u16_t chksum;
  u32_t unused;
};


/*
 * ICMPv6 Headers
 */

/* Packet Too Big */
struct icmp_ptb_hdr { 	
  u8_t type;
  u8_t icode;
  u16_t chksum;
  u32_t mtu;
};

/* NS - Neighbor Solicitation */
struct icmp_ns_hdr {    
  u8_t type; 
  u8_t icode; 
  u16_t chksum;
  u32_t reserved;
  u32_t targetip[4];
  //struct icmp_opt option;  /* for Source link-layer address */
};

/* NA - Neighbor advertisement */
struct icmp_na_hdr {    
  u8_t type; 
  u8_t icode; 
  u16_t chksum;
  u8_t rso_flags;
#define ICMP6_NA_R  0x01
#define ICMP6_NA_S  0x02
#define ICMP6_NA_O  0x04
  u8_t reserved[3];
  u32_t targetip[4];
  //struct icmp_opt option;  /* for Target link-layer address */
};

/* RS - Router Solicitation */
struct icmp_rs_hdr {    
  u8_t type; 
  u8_t icode; 
  u16_t chksum;
  u32_t reserved;
  //struct icmp_opt option;  /* for Source link-layer addres */
};

/* RA - Router Advertisement */
struct icmp_ra_hdr {    
  u8_t type; 
  u8_t icode; 
  u16_t chksum;
  u8_t hoplimit; 
  u8_t m_o_flag;
#define ICMP6_RA_M  0x80
#define ICMP6_RA_O  0x40
  u16_t life;     /* (seconds) The lifetime associated with the default router */
  u32_t reach;    /* (milliseconds) */
  u32_t retran;   /* (milliseconds) between retransmitted Neighbor Solicitation messages. */
  //struct icmp_opt option;  /* for Source link-layer addres */
  //struct icmp_opt option;  /* MTU */
  //struct icmp_opt option;  /* Prefix Information */
};


/*
 * ICMP6 Options
 */

#define ICMP6_OPT_SRCADDR     1
#define ICMP6_OPT_DESTADDR    2
#define ICMP6_OPT_PREFIX      3
#define ICMP6_OPT_REDIRECT    4
#define ICMP6_OPT_MTU         5

/* Generic ICMP option */
struct icmp_opt {       
  u8_t type;
  u8_t len;        /* in units of 8 octets (including the type and length fields). */
  u8_t data[0];    
};

/* ICMPv6 Address Options field */
struct icmp_opt_addr {       
  u8_t type;
  u8_t len;        /* in units of 8 octets (including the type and length fields). */
  u8_t addr[0];    /* 0 is not allowed with some compilers */
};

/* Length of ethernet address in 8-octects  */
#define ICMP6_OPT_LEN_ETHER   1


struct icmp_opt_prefix {       
  u8_t type;       
  u8_t len;        /* 4 */
  u8_t preflen;    /* Prefix len */
  u8_t flags;      
#define ICMP6_OPT_PREF_L  0x80
#define ICMP6_OPT_PREF_A  0x40
  u32_t valid;     
  u32_t prefered;  /* seconds */
  u32_t reserved;  /* seconds */
  u32_t prefix[4]; 
};

struct icmp_opt_mtu {       
  u8_t type;
  u8_t len;        /* 1 */
  u16_t reserved;
  u32_t mtu;    
};


#endif /* __LWIP_ICMP_H__ */
