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
#ifndef __LWIP_MEM_H__
#define __LWIP_MEM_H__

#include "lwip/opt.h"
#include "lwip/arch.h"

typedef unsigned long mem_size_t;


void mem_init(void);

void *mem_malloc(mem_size_t size);
void mem_free(void *mem);
void *mem_realloc(void *mem, mem_size_t size);
void *mem_reallocm(void *mem, mem_size_t size);
/*
#define MEM_ALIGN_1 (MEM_ALIGNMENT - 1)
#define ALIGN_SIZE(X) (((X) + MEM_ALIGN_1) & (~MEM_ALIGN_1))

#define mem_init() ({ ; })
#define mem_free(X) ({ printf("MEM-FREE %x %s %d\n",(X),__FILE__,__LINE__); \
		    free(X); })
#define mem_malloc(X) ({ void *x; x=malloc(ALIGN_SIZE(X)); \
		    printf("MEM-MALLOC %x %s %d\n",x,__FILE__,__LINE__); \
		    x; })
#define mem_realloc(Y,X) ({ void *x,*old; \
		    old=(Y);\
		    x=realloc(old,ALIGN_SIZE(X)); \
		    printf("MEM-REALLOC %x->%x %s %d\n",old,x,__FILE__,__LINE__); \
		    x; })
#define mem_reallocm(Y,X) ({ void *x,*old; \
		    old=(Y);\
		    x=realloc(old,ALIGN_SIZE(X)); \
		    printf("MEM-REALLOCM %x->%x %s %d\n",old,x,__FILE__,__LINE__); \
		    x; })
*/

#ifndef MEM_ALIGN_SIZE
#define MEM_ALIGN_SIZE(size) (((size) + MEM_ALIGNMENT - 1) & ~(MEM_ALIGNMENT-1))
#endif

#ifndef MEM_ALIGN
#define MEM_ALIGN(addr) ((void *)(((mem_ptr_t)(addr) + MEM_ALIGNMENT - 1) & ~(mem_ptr_t)(MEM_ALIGNMENT-1)))
#endif

#endif /* __LWIP_MEM_H__ */
