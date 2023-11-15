#include "execution.h"

// Quotes the braces so the wordexp ignores it
static char *newStringQuoted(char *s)
{
  // Calculate the length of the input string
  size_t inputLength = strlen(s);

  // Allocate memory for the modified string (maximum possible length is inputLength + number of added characters)
  char *modifiedString = (char *)malloc((inputLength * 3 + 1) * sizeof(char));

  if (modifiedString == NULL)
  {
    fprintf(stderr, "Memory allocation failed\n");
    exit(EXIT_FAILURE);
  }

  size_t j = 0; // Index for the modified string

  for (size_t i = 0; i < inputLength; i++)
  {
    // printf("I: %li, CHAR: %c, LEN: %li\n", i, s[i], inputLength);
    if (s[i] == '{')
    {
      // Add a single quote before the curly brace
      modifiedString[j++] = '\'';
      modifiedString[j++] = s[i];
      modifiedString[j++] = '\'';
    }
    else if (s[i] == '}')
    {
      // Add a single quote before the curly brace
      modifiedString[j++] = '\'';
      modifiedString[j++] = s[i];
      modifiedString[j++] = '\'';
    }
    else
      // Copy the current character to the modified string
      modifiedString[j++] = s[i];
  }

  // Null-terminate the modified string
  modifiedString[j] = '\0';
  // printf("||| %s\n", modifiedString);

  return modifiedString;
}

// Open file in and save as the stdin
static void subst_in(char *in)
{
  if (in != NULL)
  {
    int fd = open(in, O_RDONLY);
    if (fd == -1)
      exit(0);

    dup2(fd, STDIN_FILENO);
    close(fd);
  }
}

// Open file out and save as the stdout
static void subst_out(char *out)
{
  if (out != NULL)
  {
    int fd = open(out, O_WRONLY | O_APPEND | O_CREAT, 0666);
    if (ftruncate(fd, 0) != 0)
        perror("ftruncate() error");
    if (fd == -1)
      exit(0);

    dup2(fd, STDOUT_FILENO);
    close(fd);
  }
}

void execute(struct cmdline *l)
{
  char ***cmds = l->seq;
  char **cmd = *cmds;

  // Command expansion variables
  wordexp_t p;
  glob_t g;

  // Pipe settings
  int fd[2];
  pid_t pid;
  int fdd = 0;

  // Special cases outside of the pipe
  if (strcmp(cmd[0], "cd") == 0)
  {
    printf("NOUS SOMMES ICI: cmd[0]: %s, cmd[1]: %s\n", cmd[0], cmd[1]);
    chdir(cmd[1]);
    return;
  }
  else if (strcmp(cmd[0], "jobs") == 0)
    {
      printf("TACHE DE FONDS: \n");
      tf_node *current = head_fond;
      tf_node *previous = NULL;

      while (current != NULL)
      {
        if (kill(current->tache->pid, 0) == 0)
        {
          printf("  PID: %i | CMD: %s\n", current->tache->pid, current->tache->cmd);
          previous = current;
          current = current->next;
        }
        else
        {
          if (previous == NULL)
          {
            tf_node *next = current->next;
            free(current->tache);
            free(current);
            current = next;
            head_fond = NULL;
          }
          else
          {
            tf_node *next = current->next;
            free(current->tache);
            free(current);
            previous->next = next;
            current = next;
          }
        }
      }
    }


  if(l->bg) // If an command should run in the background
    executecmdFond(cmd, l->in, l->out, &p, &g);
  else 
  
  // Variante
  // Pipes multiples
  while (*cmds != NULL) // Execute the commands in the pipe
  {
    // Creates pipe and do the fork
    if(pipe(fd))
    {
      perror("error creating pipe");
    }

    pid = fork();
    if (pid == -1)
    {
      perror("fork");
      exit(1);
    }
    else if (pid == 0) // The child executes the command
    {
      // Read the previous input
      dup2(fdd, 0);
      if (*(cmds + 1) != NULL) // If the command isn't the last one
      {
        // Replace standard output as the pipe
        dup2(fd[1], 1);
      }
      close(fd[0]);

      executecmd(*cmds, l->in, l->out, &p, &g);
    }
    else
    {
      if(wait(NULL)==-1)
      {
        perror("no child to wait");
      }
      close(fd[1]);

      // Save the previous read end
      fdd = fd[0];
      
      // Increment to the next command of the pipe
      cmds++;
    }
  }
}

void executecmd(char **cmd, char *in, char *out, wordexp_t *p, glob_t *g)
{
  // Expand
  cmd = expandJoker(cmd, p, g);

  // Substitute  the i/o of the command
  subst_in(in);
  subst_out(out);

  // Command execution
  execvp(cmd[0], cmd);
  exit(1);
}

void executecmdFond(char **cmd, char *in, char *out, wordexp_t *p, glob_t *g)
{
  // Expand
  cmd = expandJoker(cmd, p, g);

  pid_t pid = fork();
  if (pid == 0)
  {
    // Substitute  the i/o of the command
    subst_in(in);
    subst_out(out);
    execvp(cmd[0], cmd);
    exit(1);
  }
  else
  {
    if (pid < 0)
    {
      perror("fork: ");
      return;
    }
    tf_node *node = (tf_node *)malloc(sizeof(tf_node));
    if (node == NULL)
      return;

    node->next = head_fond;

    node->tache = (tacheFond *)malloc(sizeof(tacheFond));
    if (node->tache == NULL)
      free(node);
    else
    {
      node->tache->cmd = (char *)malloc(sizeof(char) * MAX_CMD_SIZE);
      if (node->tache->cmd == NULL)
      {
        free(node->tache);
        free(node);
      }
      else
      {
        strcpy(node->tache->cmd, cmd[0]);
        node->tache->pid = pid;

        head_fond = node;
      }
    }
  }
}

char **expandJoker(char **cmd, wordexp_t *p, glob_t *g)
{
  // Variante
  // Joker en wordexp and Joker en glob

  // Expand the entire cmd
  switch (wordexp(cmd[0], p, 0))
  {
  case 0: /* Successful.  */
    break;
  case WRDE_NOSPACE:
    /* If the error was WRDE_NOSPACE,
      then perhaps part of the result was allocated.  */
    printf("ERROP SPACE\n");
    wordfree(p);
  default: /* Some other error.  */
    printf("ERRO");
    return cmd;
  }

  // Expand the entire cmd
  switch (glob(cmd[0], GLOB_NOCHECK, NULL, g))
  {
  case 0: /* Successful.  */
    break;
  case GLOB_NOSPACE:
    /* If the error was GLOB_NOSPACE,
      then perhaps part of the result was allocated.  */
    printf("ERROG SPACE\n");
    globfree(g);
  default: /* Some other error.  */
    printf("BAH zou");
    return cmd;
  }

  /* Expand the strings specified for the arguments.  */
  for (int i = 1; cmd[i] != NULL; i++)
  {
    char *formatted = newStringQuoted(cmd[i]);
    int err = wordexp(formatted, p, WRDE_APPEND);
    if (err)
    {
      printf("ERROP %d\n", err);
      wordfree(p);
      free(formatted);
      return cmd;
    }
    free(formatted);
  }

  for (int i = 1; i < p->we_wordc; i++)
  {
    int err = glob(p->we_wordv[i], GLOB_APPEND | GLOB_NOCHECK | GLOB_BRACE, NULL, g);
    if (err)
    {
      printf("ERROG %d\n", err);
      globfree(g);
      return cmd;
    }
  }

  return g->gl_pathv;
}
