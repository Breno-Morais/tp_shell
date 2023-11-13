#ifndef __EXECUTION_H
#define __EXECUTION_H

#include "readcmd.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <wordexp.h>
#include <glob.h>

#define MAX_CMD_SIZE 10

void execute(struct cmdline *l);
void executecmd(char** cmd, char *in, char *out, wordexp_t *p, glob_t *g);
void executecmdFond(char **cmd, wordexp_t *p, glob_t *g);
void executePipe(char **cmd, int fd[2], int i, wordexp_t *p, glob_t *g);
char **expandJoker(char **cmd, wordexp_t *p, glob_t *g);

typedef struct _TACHE_FOND {
  int pid;
  char* cmd;
} tacheFond;

typedef struct _TF_LINKED_NODE {
  tacheFond* tache;
  struct _TF_LINKED_NODE* next;
} tf_node;

extern tf_node *head_fond;

#endif