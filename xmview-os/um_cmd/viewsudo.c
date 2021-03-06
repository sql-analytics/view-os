/*   This is part of um-ViewOS
 *   The user-mode implementation of OSVIEW -- A Process with a View
 *
 *   viewsudo user command
 *   
 *   Copyright 2010 Renzo Davoli University of Bologna - Italy
 *   
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License, version 2 or 
 *   (at your option) any later version, as published by the Free Software Foundation.
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
 *   $Id: um_add_service.c 775 2009-09-01 21:15:23Z rd235 $
 *
 */   
#include <config.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <getopt.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <libgen.h>
#include <um_lib.h>

static struct option long_options[] = {
	{"help", 1, 0, 'h'},
	{0, 0, 0, 0}
};

char *command;
char *user;
char *group;
int uid;
int gid;
int ngroups;
gid_t *groups;

void usage(char *argv0)
{
	char *name=basename(argv0);
	fprintf(stderr,
			"Usage: %s [options] [LOGIN]\n"
			"\n"
			"Options:\n"
			"  -u  username|#uid\n"
			"  -g  groupname|#gid\n"
			"\n",
			name
			);
	exit(2);
}

int main(int argc, char *argv[])
{
	pid_t pid;
	int status;
	int c;
	struct passwd *pwd;

	/* outside viewos use sudo(1) */
	if (um_check_viewos()==0) 
		execvp("sudo",argv);

	while (1) {
		int option_index = 0;
		c = getopt_long(argc, argv, "u:g:h",
				long_options, &option_index);
		if (c == -1)
			break;
		switch (c) {
			case 'u':user=optarg;
							 break;
			case 'g':group=optarg;
							 break;
			case 'h': usage(argv[0]);
								break;
		}
	}
	if (argc==optind)
		usage(argv[0]);

	if (user) {
		if (*user == '#')
			uid=atoi(user+1);
		else {
			pwd=getpwnam(user);
			if (pwd == NULL) {
				fprintf(stderr,"Unknown id: %s\n",user);
				exit(1);
			}
			uid=pwd->pw_uid;
		}
	} else {
		if (group)
			uid=getuid();
	}
	if (pwd==NULL)
		pwd=getpwuid(uid);
	if (pwd) {
		gid=pwd->pw_gid;
		setenv("HOME",pwd->pw_dir,1);
	} 
	if (group) {
		if (*group == '#')
			gid=atoi(group+1);
		else {
			struct group *grp=getgrnam(group);
			if (grp == NULL) {
				fprintf(stderr,"Unknown group id: %s\n",group);
				exit(1);
			}
			gid=grp->gr_gid;
		}
	}

	if (pwd) {
		getgrouplist(pwd->pw_name,gid,NULL,&ngroups);
		groups=malloc(ngroups * sizeof (gid_t));
		if (groups == NULL)
			ngroups=0;
		else
			getgrouplist(pwd->pw_name,gid,groups,&ngroups);
	}

	switch (pid=fork()) {
		case -1: exit(1);
		case 0: 
						 if (setresuid(uid,uid,uid) < 0)
							 perror(argv[0]);
						 else {
							 setresgid(gid,gid,gid);
							 setgroups(ngroups,groups);
							 execvp(argv[optind],argv+optind);
							 perror(argv[optind]);
						 }
						 exit(1);
						 break;
		default:
						 waitpid(pid,&status,0);
						 return(WEXITSTATUS(status));
	}
	return 0;
}
