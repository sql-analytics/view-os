/*   This is part of um-ViewOS
 *   The user-mode implementation of OSVIEW -- A Process with a View
 *
 *   um_add_service user command
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
 *   $Id$
 *
 */   
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <config.h>
#include <um_lib.h>

void usage()
{
	fprintf(stderr, "Usage:\n\tum_add_service service_dyn_library\n");
	exit(2);
}

int main(int argc, char *argv[])
{
	int c;
	int permanent=0;
	if (um_check_viewos()==0) {
		fprintf(stderr,"This is a View-OS command. It works only inside a umview/kmview virtual machine\n");
		usage();
	}
	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"permanent",0,0,'p'},
			{0,0,0,0}
		};
		c=getopt_long(argc,argv,"p",long_options,&option_index);
		if (c == -1) break;
		switch (c) {
			case 'p': permanent=1;
								break;
		}
	}
	if (argc - optind != 1)
		usage();
	else {
		if (um_add_service(argv[optind],permanent) < 0) {
			perror("um_add_service");
			exit(1);
		}
		else
			exit(0);
	}
	return 0;
}

