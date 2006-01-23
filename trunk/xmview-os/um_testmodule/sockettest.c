/*   This is part of um-ViewOS
 *   The user-mode implementation of OSVIEW -- A Process with a View
 *
 *   example of um-ViewOS module
 *   
 *   Copyright 2005 Renzo Davoli University of Bologna - Italy
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
 *   $Id$
 *
 */   
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <linux/net.h>
#include <string.h>
#include "module.h"
#include "libummod.h"

// int read(), write(), close();

static struct service s;

static int choiceissocket(int type,void *arg)
{
	if (type==CHECKSOCKET)
		return 1;
	else
		return 0;
}

static void
__attribute__ ((constructor))
init (void)
{
	printf("sockettest init\n");
	s.name="sockettest (syscall are executed server side)";
	s.code=0xfa;
	s.checkfun=choiceissocket;
	s.syscall=(intfun *)malloc(scmap_scmapsize * sizeof(intfun));
	s.socket=(intfun *)malloc(scmap_sockmapsize * sizeof(intfun));
	s.socket[SYS_SOCKET]=socket;
	s.socket[SYS_BIND]=bind;
	s.socket[SYS_CONNECT]=connect;
	s.socket[SYS_LISTEN]=listen;
	s.socket[SYS_ACCEPT]=accept;
	s.socket[SYS_GETSOCKNAME]=getsockname;
	s.socket[SYS_GETPEERNAME]=getpeername;
	s.socket[SYS_SEND]=send;
	s.socket[SYS_RECV]=recv;
	s.socket[SYS_SENDTO]=sendto;
	s.socket[SYS_RECVFROM]=recvfrom;
	s.socket[SYS_SHUTDOWN]=shutdown;
	s.socket[SYS_SETSOCKOPT]=setsockopt;
	s.socket[SYS_GETSOCKOPT]=getsockopt;
	s.socket[SYS_SENDMSG]=sendmsg;
	s.socket[SYS_RECVMSG]=recvmsg;
	s.syscall[uscno(__NR_read)]=read;
	s.syscall[uscno(__NR_write)]=write;
	s.syscall[uscno(__NR_readv)]=readv;
	s.syscall[uscno(__NR_writev)]=writev;
        s.syscall[uscno(__NR_close)]=close;
	s.syscall[uscno(__NR_fcntl)]=fcntl32;
#if !defined(__x86_64__)
	s.syscall[uscno(__NR_fcntl64)]=fcntl64;
#endif
	s.syscall[uscno(__NR_ioctl)]=(intfun) ioctl;
	s.syscall[uscno(__NR__newselect)]=select;
	s.syscall[uscno(__NR_poll)]=poll;

	add_service(&s);
}

static void
__attribute__ ((destructor))
fini (void)
{
	free(s.syscall);
	free(s.socket);
	printf("sockettest fini\n");
}
