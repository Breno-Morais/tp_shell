#include "execution.h"

void executecmd(char **cmdArgs)
{  
  char *cmd = cmdArgs[0];
  char **args = (char **) ++cmdArgs;

  pid_t pid = fork();
  if(pid == 0)
  {
    execvp(cmd, args);
  } else {
    if (wait(NULL)==-1){
      perror("wait: ");
    }
    // int status = -1;
    // pid_t child_pid = wait(&status);
    // if(child_pid == -1) {
    //   printf("\ner\n");
    // }
  }
}