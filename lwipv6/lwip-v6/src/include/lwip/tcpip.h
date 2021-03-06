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
#ifndef __LWIP_TCPIP_H__
#define __LWIP_TCPIP_H__

#include "lwip/pbuf.h"
#include "lwip/stack.h"
#if LWIP_CAPABILITIES
typedef int (*lwip_capfun)(void);
#endif

enum tcpip_msg_type {

  /* Core messages */
  TCPIP_MSG_API,
  TCPIP_MSG_INPUT,
  TCPIP_MSG_CALLBACK,
  TCPIP_MSG_SYNC_CALLBACK,

  /* other messages */
  TCPIP_MSG_NETIFADD,
  TCPIP_MSG_SHUTDOWN,
  TCPIP_MSG_NETIF_NOTIFY
};

enum tcpip_sync {
	ASYNC = 0,
	SYNC = 1
};

struct tcpip_msg {

  enum tcpip_msg_type type;
  sys_sem_t *sem;                  // FIX: for what?????????

  union {

    struct api_msg *apimsg;

    struct {
      struct pbuf *p;
      struct netif *netif;
    } inp;

    struct {
      sys_sem_t *sem;    // used for synchronous calls
      void (*f)(void *ctx);
      void *ctx;
    } cb;


    /* Signal to the main thread to add a new network interface */
    struct {
      sys_sem_t *sem;    // used for synchronous calls
      struct netif *netif;
      void *state;
      err_t (* init)(struct netif *netif);
      err_t (* input)(struct pbuf *p, struct netif *netif);
      void (* change)(struct netif *netif, u32_t type);
      struct netif **retval;
    } netif;


    struct {
      sys_sem_t *sem;    // used for synchronous calls
      struct netif *netif;
      u32_t type;
    } netif_notify;

  } msg;

};

int tcpip_init(void);

typedef void (* tcpip_handler)(void *arg);

#if LWIP_CAPABILITIES
struct stack *tcpip_start(tcpip_handler init_func, void *arg, unsigned long flags, lwip_capfun capfun);
#else
/* Alloc a new stack thread and return stack number */
struct stack *tcpip_start(tcpip_handler init_func, void *arg, unsigned long flags);
#endif

/* Signal to the stack to shutdown */
void tcpip_shutdown(struct stack *stack, tcpip_handler shutdown_func, void *arg);

/* Get "current" stack */
struct stack *tcpip_stack_get(void);

/* Set "current" stack */
struct stack *tcpip_stack_set(struct stack *id);


/* These functions send messages to the stack thread.
   After tcpip_shutdown() they are unuseful. */
void  tcpip_apimsg(struct stack *stack, struct api_msg *apimsg);
err_t tcpip_input(struct pbuf *p, struct netif *inp);
err_t tcpip_callback(struct stack *stack, void (*f)(void *ctx), void *ctx, enum tcpip_sync sync);

void tcpip_tcp_timer_needed(struct stack *stack);

/* Tell to the stack to create a new interface. This function
   should be used only when you can't create interfaces before
   the launch of the stack thread. 
   N.B: this functions has been created for the ViewOS project. */
struct netif * tcpip_netif_add(
      struct stack *stack, 
      struct netif *netif, 
      void *state,
      err_t (* init)(struct netif *netif),
      err_t (* input)(struct pbuf *p, struct netif *netif),
      void  (* change)(struct netif *netif, u32_t type));

/* Signal to the stack the interface's state is changed */
void tcpip_notify(struct netif *netif, u32_t type);


#endif /* __LWIP_TCPIP_H__ */
