#include "execution.h"

void execute(struct cmdline *l)
{
  /* Main pipe creation */
  int fd[2];
  if(pipe(fd) != 0)
  {
    printf("Error creating pipe.");
    exit(0);
  }

  char **cmd = l->seq[0];
  wordexp_t p;
  glob_t g;

  /* Display both command of the pipe */
  if(l->seq[1] != 0) { // Execute the pipe
    char **cmd2 = l->seq[1];

    pid_t pid = fork();

    if(pid == 0)
    {
      executePipe(cmd, fd, 1, &p, &g);
    } else { // Father continues
      pid_t pid2 = fork();

      if(pid2 == 0) // Segunda criança
      {  
        executePipe(cmd2, fd, 0, &p, &g);
      } else { // Ainda o pai lá
        if (wait(NULL)==-1){
          perror("wait: ");
        }    
      }
    }
  } else {
    close(fd[0]);
    close(fd[1]);

    if(l->bg)
      executecmdFond(cmd, &p, &g);
    else
      executecmd(cmd, l->in, l->out, &p, &g);
  }

  //wordfree(&p);
  //globfree(&g);
}	

void executecmd(char **cmd, char *in, char *out, wordexp_t *p, glob_t *g)
{ 
  // Expand 
  cmd = expandJoker(cmd, p, g);

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

    // Command execution
    execvp(cmd[0], cmd);
    exit(0);
  } else { // Parent
    if (wait(NULL)==-1){
      perror("wait: ");
    }
  }
}

void executecmdFond(char **cmd, wordexp_t *p, glob_t *g)
{  
  // Expand 
  cmd = expandJoker(cmd, p, g);

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

void executePipe(char **cmd, int fd[2], int i, wordexp_t *p, glob_t *g)
{
  // Expand 
  cmd = expandJoker(cmd, p, g);

  // Child updates the pipe
  dup2(fd[i], i); // Replace standard output of child process with write end of the pipe
  close(fd[0]); // Close the write end
  close(fd[1]); // Close read end

  execvp(cmd[0], cmd);
  exit(0);
}

char **expandJoker(char **cmd, wordexp_t *p, glob_t *g)
{
  // Variante
  // Joker expansion

  // Expand the entire cmd
  switch (wordexp(cmd[0], p, 0))
  {
    case 0:			/* Successful.  */
      break;
    case WRDE_NOSPACE:
      /* If the error was WRDE_NOSPACE,
        then perhaps part of the result was allocated.  */
      printf("ERROP SPACE\n");
      wordfree (p);
    default:                    /* Some other error.  */
      printf("ERRO");
      return cmd;
  }

  // Expand the entire cmd
  switch (glob(cmd[0], GLOB_NOCHECK, NULL, g))
  {
    case 0:			/* Successful.  */
      break;
    case GLOB_NOSPACE:
      /* If the error was GLOB_NOSPACE,
        then perhaps part of the result was allocated.  */
      printf("ERROG SPACE\n");
      globfree (g);
    default:                    /* Some other error.  */
      printf("BAH zou");
      return cmd;
  }

  /* Expand the strings specified for the arguments.  */
  for (int i = 1; cmd[i] != NULL; i++)
  {
    int err = glob(cmd[i], GLOB_APPEND | GLOB_NOCHECK | GLOB_BRACE, NULL, g);
    if(err)
    {
      printf("ERROG %d\n", err);
      globfree (g);
      return cmd;
    }
  }

  for (int i = 1; i < g->gl_pathc; i++) 
  {
    int err = wordexp (g->gl_pathv[i], p, WRDE_APPEND);
    if (err)
      {
        printf("ERROP %d\n", err);
        wordfree (p);
        return cmd;
      }
  }

  return g->gl_pathv;
}