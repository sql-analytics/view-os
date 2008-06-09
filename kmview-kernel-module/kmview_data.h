/*
 * kmview data structures
 *
 * Copyright (C) 2007 Andrea Gasparini (gaspa@yattaweb.it), 
 *                    Renzo Davoli (renzo@cs.unibo.it)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Due to this file being licensed under the GPL there is controversy over
 *  whether this permits you to write a module that #includes this file
 *  without placing your module under the GPL.  Please consult a lawyer for
 *  advice before doing this.
 *
 */

#ifndef _KMVIEW_DATA_H
#define _KMVIEW_DATA_H
#include <linux/ioctl.h>
#include <linux/utrace.h>
#include <linux/list.h>
#include <linux/wait.h>
/* PUBLIC INTERFACE IS IN kmview.h */
#include "kmview.h"

#define USE_KMEM_CACHE

struct kmview_tracer {
	struct semaphore sem;
	struct task_struct *task;
	pid_t ntraced;
	u32 flags;
	long magicpoll_addr;
	long magicpoll_cnt;
	wait_queue_head_t event_waitqueue;
	struct list_head event_queue;
};

#define KMVIEW_THREAD_FLAG_SKIP_EXIT 1
#define KMVIEW_THREAD_FLAG_SKIP_CALL 2
#define KMVIEW_THREAD_FLAG_SKIP_BOTH 3

struct kmview_thread {
	struct task_struct *task;
	struct kmview_tracer *tracer;
	pid_t kmpid;
	pid_t umpid;
	u32 flags;
	unsigned long scno;
	struct utrace_attached_engine *engine;
	struct pt_regs *regs;
#ifdef __NR_socketcall
	unsigned long socketcallargs[6];
#endif
	struct kmview_fdsysset *fdset;
};


struct kmview_module_event {
	struct list_head eventlist;
	u32 tag;
	struct kmview_thread *thread;
	pid_t arg; /* umppid or ntraced */
	unsigned long arg2; /*clone_flags*/
};
#endif