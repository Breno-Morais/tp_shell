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

#define MAX_CMD_SIZE 10

void executecmd(char** cmd, char *in, char *out);
void executecmdFond(char **cmd);
void executePipe(char **cmd, int fd[2], int i);

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