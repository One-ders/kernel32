/* $FrameWorks: , v1.1 2014/04/07 21:44:00 anders Exp $ */

/*
 * Copyright (c) 2014, Anders Franzen.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @(#)sys_cmd.c
 */
#include "io.h"
#include "sys.h"

#include <string.h>

extern struct cmd_node my_cmd_node;

struct cmd_node *root_cmd_node=&my_cmd_node;
static struct cmd_node *current_node=&my_cmd_node;

int install_cmd_node(struct cmd_node *new,struct cmd_node *parent) {
	new->next=parent->next;
	parent->next=new;
	return 0;
}

int generic_help_fnc(int argc, char **argv, struct Env *env) {
	struct cmd *cmd=current_node->cmds;
	struct cmd_node *cn=current_node->next;
	fprintf(env->io_fd, "help called with %d args\n", argc);
	fprintf(env->io_fd, "available commands:\n");
	while(cmd->name) {
		fprintf(env->io_fd, "%s, ", cmd->name);
		cmd++;
	}

	while(cn){
		fprintf(env->io_fd, "%s, ", cn->name);
		cn=cn->next;
	}

	fprintf(env->io_fd, "\n");
	return 0;
}


struct cmd *lookup_cmd(char *name, int fd) {
	struct cmd *cmd=current_node->cmds;
	struct cmd_node *cn=current_node->next;
	while(cmd->name) {
		if (__builtin_strcmp(cmd->name,name)==0) {
			return cmd;
		}
		cmd++;
	}
	
	while(cn) {
		if (__builtin_strcmp(cn->name,name)==0) {
			current_node=cn;
			return 0;
		}
		cn=cn->next;
	}

	if (__builtin_strcmp("exit",name)==0) {
		struct cmd_node *cn=root_cmd_node->next;
		struct cmd_node *prevcn=root_cmd_node;

		while(cn&&(cn!=current_node)) {
			prevcn=cn;
			cn=cn->next;
		}

		if (cn) {
			current_node=prevcn;
		}
		return 0;
	}
	fprintf(fd,"cmd %s, not found\n", name);
	return 0;
}

int argit(char *str, int len, char *argv[16]) {
	int ac=0;
	char *p=str;
	while(1) {
		while((*p)==' ') {
			p++;
			if (p>=(str+len)) {
				return ac;
			}
		}
		argv[ac]=p;
		while((*p)!=' ') {
			p++;
			if (p>=(str+len)) {
				ac++;
				return ac;
			}		
		}
		*p=0;
		p++;
		ac++;
		if (ac>=16) {
			return 0;
		}
	}
	return ac;
}

int argc;
char *argv[16];

void sys_mon(void *dum) {
	char *buf=getSlab_256();
	int fd=io_open((char *)dum);
	struct Env env;
	if (fd<0) return;
	env.io_fd=fd;
	io_write(fd,"Starting sys_mon\n",17);

	while(1) {
		int rc;
		rc=readline_r(fd,"\n--->",buf,200);
		if (rc>0) {
			struct cmd *cmd;
			if (rc>200) {
				rc=200;
			}
			buf[rc]=0;
			rc=argit(buf,rc,argv);
			if (rc<0) {
				continue;
			}
			argc=rc;
			cmd=lookup_cmd(argv[0],fd);
			if (cmd) {
				int rc;
				fprintf(fd,"\n");
				rc=cmd->fnc(argc,argv,&env); 
				if (rc<0) {
					fprintf(fd,"%s returned %d\n",rc);
				}
			}
		}
	}

}
