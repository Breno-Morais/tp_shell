#include "execution.h"

void executecmd(char **cmd, char *in, char *out)
{  
  // Special cases
  if(strcmp(cmd[0], "cd") == 0) 
  {
    chdir(cmd[1]);
    return;
  } else if(strcmp(cmd[0], "jobs") == 0)
  {
    printf("TACHE DE FONDS: \n");
    tf_node* current = head_fond;
    tf_node* previous = NULL;

    while(current != NULL)
    {
      if(kill(current->tache->pid,0) == 0)
      {
        printf("  PID: %i | CMD: %s\n", current->tache->pid, current->tache->cmd);
        previous = current;
        current = current->next;
      } else 
      {
        if(previous == NULL)
        {
          tf_node* next = current->next;
          free(current->tache);
          free(current);
          current = next;
          head_fond = NULL;
        } else {
          tf_node* next = current->next;
          free(current->tache);
          free(current);
          previous->next = next;
          current = next;
        }
      }
    }
  }
  
  pid_t pid = fork();
  if(pid == 0) // Child
  {
    if(out != NULL)
    {
      int fd = open(out, O_WRONLY | O_APPEND | O_CREAT, 0666);    
      if(fd == -1)
        exit(0);

      dup2(fd, STDOUT_FILENO);
      close(fd);
    }
    if(in != NULL)
    {
      int fd = open(in, O_RDONLY);
      if(fd == -1)
        exit(0);

      dup2(fd, STDIN_FILENO);
      close(fd);
    }

    execvp(cmd[0], cmd);
    exit(0);
  } else { // Parent
    if (wait(NULL)==-1){
      perror("wait: ");
    }
  }
}

void executecmdFond(char **cmd)
{  
  pid_t pid = fork();
  if(pid == 0)
  {
    execvp(cmd[0], cmd);
    exit(0);
  } else {
    if(pid < 0) 
    {
      perror("fork: ");
      return;
    }

    tf_node* node = (tf_node*) malloc(sizeof(tf_node));
    if(node == NULL) return;

    node->next = head_fond;

    node->tache = (tacheFond*) malloc(sizeof(tacheFond));
    if(node->tache == NULL)
      free(node);
    else {
      node->tache->cmd = (char *) malloc(sizeof(char) * MAX_CMD_SIZE);
      if(node->tache->cmd == NULL)
      {
        free(node->tache);
        free(node);
      } else {
        strcpy(node->tache->cmd, cmd[0]);
        node->tache->pid = pid;

        head_fond = node;
      }
    }
  }
}

void executePipe(char **cmd, int fd[2], int i)
{
  // Child updates the pipe
  dup2(fd[i], i); // Replace standard output of child process with write end of the pipe
  close(fd[0]); // Close the write end
  close(fd[1]); // Close read end

  execvp(cmd[0], cmd);
  exit(0);
}