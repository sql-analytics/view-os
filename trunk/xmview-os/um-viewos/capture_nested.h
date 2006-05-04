/*   This is part of um-ViewOS
 *   The user-mode implementation of OSVIEW -- A Process with a View
 *
 *   Copyright 2006 Renzo Davoli University of Bologna - Italy
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
#ifndef CAPTURE_NESTED_H_
#define CAPTURE_NESTED_H_
#include <sys/stat.h>
#include "treepoch.h"


struct npcb {
	short flags;
	long scno;              /* System call number */
	long args[8];
	long erno;
	void *path;
	struct stat64 pathstat;
	struct timestamp tst;
	epoch_t nestepoch;
};
		
union xpcb_key {
	struct pcb *pcb;
	struct npcb *npcb;
};

void capture_nested_init(void);

#endif