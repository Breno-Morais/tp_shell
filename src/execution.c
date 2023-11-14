#include "execution.h"

static char *newStringQuoted(char *s)
{
	// Calculate the length of the input string
	size_t inputLength = strlen(s);

	// Allocate memory for the modified string (maximum possible length is inputLength + number of added characters)
	char* modifiedString = (char*)malloc((inputLength * 3 + 1) * sizeof(char));

	if (modifiedString == NULL) {
			fprintf(stderr, "Memory allocation failed\n");
			exit(EXIT_FAILURE);
	}

	size_t j = 0; // Index for the modified string

	for (size_t i = 0; i < inputLength; i++) {
					//printf("I: %li, CHAR: %c, LEN: %li\n", i, s[i], inputLength);
			if (s[i] == '{') {
					// Add a single quote before the curly brace
					modifiedString[j++] = '\'';
					modifiedString[j++] = s[i];
					modifiedString[j++] = '\'';		
			} else if (s[i] == '}') {
					// Add a single quote before the curly brace
					modifiedString[j++] = '\'';
					modifiedString[j++] = s[i];
					modifiedString[j++] = '\'';		
			} else
				// Copy the current character to the modified string
				modifiedString[j++] = s[i];			
	}

	// Null-terminate the modified string
	modifiedString[j] = '\0';
	//printf("||| %s\n", modifiedString);

	return modifiedString;
}

static void subst_in(char *in)
{
  if(in != NULL)
  {
    int fd = open(in, O_RDONLY);
    if(fd == -1)
      exit(0);

    dup2(fd, STDIN_FILENO);
    close(fd);
  }
}

static void subst_out(char *out)
{
  if(out != NULL)
  {
    int fd = open(out, O_WRONLY | O_APPEND | O_CREAT, 0666);    
    if(fd == -1)
      exit(0);

    dup2(fd, STDOUT_FILENO);
    close(fd);
  }
}

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
  //int saved_stdout = dup(1); // The terminal output

  if(l->seq[1] != 0) { // Execute the pipe
    pid_t pid0 = fork();

    if(pid0 == 0) // Child does the 
    {
      int i;
      for(i = 0; l->seq[i + 1] != NULL; i++)
      {
        cmd = l->seq[i];

        pid_t pid = fork();

        if(pid < 0) {
          printf("Erro no forkn\n");
          exit(0);
        } else if(pid == 0)
        {
          // Expand 
          cmd = expandJoker(cmd, &p, &g);

          // Child updates the pipe
          dup2(fd[1], 1); // Replace standard output of child process with write end of the pipe

          close(fd[0]); // Close the write end
          close(fd[1]); // Close read end

          // Substitute  the i/o of the command
          subst_in(l->in);
          subst_out(l->out);

          printf("1)DUP: (in:%d, out: %d)\n", dup(0), dup(1));
          execvp(cmd[0], cmd);
          exit(0);
        } else { // Father continues
          if (wait(NULL)==-1){
            perror("wait: ");
          }

          // Parent updates the pipe
          dup2(fd[0], 0); // Replace standard input of father process with read end of the pipe
          close(fd[0]); // Close the write end
          close(fd[1]); // Close read end

          printf("2)DUP: (in:%d, out: %d)\n", dup(0), dup(1));

          //fd = calloc(2, sizeof(int));
          if(pipe(fd) != 0)
          {
            printf("Error creating pipe.");
            exit(0);
          }
        }
      }

      if (wait(NULL)==-1){
        perror("wait: ");
      }

      printf("3)DUP: (in:%d, out: %d)\n", dup(0), dup(1));
      executecmd(l->seq[i], l->in, l->out, &p, &g);
      exit(0);
    } else {
      if (wait(NULL)==-1){
        perror("wait: ");
      }
    }
  } else {
    close(fd[0]);
    close(fd[1]);

    if(l->bg)
      executecmdFond(cmd, l->in, l->out, &p, &g);
    else
      executecmd(cmd, l->in, l->out, &p, &g);
  }

  //close(saved_stdout);
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
    // Substitute  the i/o of the command
    subst_in(in);
    subst_out(out);

    // Command execution
    execvp(cmd[0], cmd);
    exit(0);
  } else { // Parent
    if (wait(NULL)==-1){
      perror("wait: ");
    }
  }
}

void executecmdFond(char **cmd, char *in, char *out, wordexp_t *p, glob_t *g)
{  
  // Expand 
  cmd = expandJoker(cmd, p, g);

  pid_t pid = fork();
  if(pid == 0)
  {
    // Substitute  the i/o of the command
    subst_in(in);
    subst_out(out);
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
    char *formatted = newStringQuoted(cmd[i]);
    int err = wordexp (formatted, p, WRDE_APPEND);
    if (err)
      {
        printf("ERROP %d\n", err);
        wordfree (p);
        free(formatted);
        return cmd;
      }
    free(formatted);
  }

  for (int i = 1; i < p->we_wordc; i++) 
  {
    int err = glob(p->we_wordv[i], GLOB_APPEND | GLOB_NOCHECK | GLOB_BRACE, NULL, g);
    if(err)
    {
      printf("ERROG %d\n", err);
      globfree (g);
      return cmd;
    }
  }

  return g->gl_pathv;
}
