#ifndef __EXECUTION_H
#define __EXECUTION_H

#include "readcmd.h"
#include <unistd.h>
#include <sys/wait.h>

void executecmd(char** cmdArgs);

#endif