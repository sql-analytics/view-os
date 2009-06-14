/*   This is part of um-ViewOS
 *   The user-mode implementation of OSVIEW -- A Process with a View
 *
 *   um_signal: basic signal handling (to be completed).
 *   added to manage signals to blocked processes.
 *   
 *   Copyright 2005 Renzo Davoli University of Bologna - Italy
 *   
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License, version 2, as
 *   published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA.
 *
 */

#include <config.h>
#include "defs.h"
#include "services.h"
#include "mainpoll.h"

int wrap_in_kill(int sc_number,struct pcb *pc,
		    service_t sercode, sysfun um_syscall)
{
	long pid=pc->sysargs[0];
	if (bq_pidwake(pid,pc->sysargs[1])) {
		putscno(__NR_getpid,pc);
		return SC_MODICALL;
	} else
		return STD_BEHAVIOR;
}

int wrap_out_kill(int sc_number,struct pcb *pc)
{
	return STD_BEHAVIOR;
}
