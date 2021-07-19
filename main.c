#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <signal.h>

#define clear() printf("\033[H\033[J")

#define SHELL_BUFSIZE 1024

#define SHELL_TOK_BUFSIZE 64
#define SHELL_TOK_DELIM " \t\r\n\a"

char** shell_split_line(char* line) {
  
  int bufsize = SHELL_TOK_BUFSIZE;
  int position = 0;
  char** tk_list = malloc(bufsize * sizeof(char*));
  char* token;
  
  if(!tk_list) {
    fprintf(stderr, "buffer allocation error\n");
    exit(1); // EXIT_FAILURE
  }
  
  token = strtok(line, SHELL_TOK_DELIM);
  while(token != NULL) {
    tk_list[position++] = token;
    
    if(position >= bufsize) {
      bufsize += SHELL_TOK_BUFSIZE;
      tk_list = realloc(tk_list, bufsize * sizeof(char*));
      if(!tk_list) {
        fprintf(stderr, "buffer allocation error\n");
        exit(1); // EXIT_FAILURE
      }
    }
    token = strtok(NULL, SHELL_TOK_DELIM);
  }
  tk_list[position] = NULL;
  return tk_list;
}

int shell_launch(char** args) {
  pid_t pid, wpid;
  int status;
  
  pid = fork();
  if(pid == 0) { // child process
    // replace the current running program with exec
    // 'p' let the operating system search for the program path
    // 'v' pass a list of string args to it
    if(execvp(args[0], args) == -1) {
      perror("shell");
    }
    exit(1); // EXIT_FAILURE
     
  } else if(pid < 0) {
    // error forking
    perror("shell");
  } else { // parent process
    do {
      wpid = waitpid(pid, &status, WUNTRACED);
      // wait until the process are exited or killed
    } while(!WIFEXITED(status) && !WIFSIGNALED(status));
  }
  return 1; // launch success
}


int shell_cd(char** args);
int shell_help(char** args);
int shell_exit(char** args);

char* builtin_str[] = {
  "cd",
  "help",
  "exit"
};

int (*builtin_func[]) (char**) = {
  &shell_cd,
  &shell_help,
  &shell_exit
};

int shell_num_builtins() {
  return sizeof(builtin_str) / sizeof(char*);
}

int shell_cd(char** args) {
  if(args[1] == NULL) {
    fprintf(stderr, "shell: expected argument to \'cd\'\n");
  } else {
    if(chdir(args[1]) != 0) {
      perror("shell");
    }
  }
  return 1;
}

int shell_help(char** args) {

  printf("\x1b[1;33m");
  printf("Shell");
  printf("\x1b[0m\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");
  
  printf("\x1b[1;31m");
  for(int i = 0; i < shell_num_builtins(); i++) {
    printf(" %s\n", builtin_str[i]);
  }
  printf("\x1b[0m");
  printf("Use the\x1b[1;31m man\x1b[0m command for information on other programs.\n");
  return 1;
}

int shell_exit(char** args) {
  return 0;
}

int shell_execute(char** args) {
  
  if(args[0] == NULL) {
    // an empty command
    return 1;
  }
  for(int i = 0; i < shell_num_builtins(); i++) {
    if(strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }
  return shell_launch(args);
}

#define MAX_PROMPT_SIZE 1024
#define MAX_CWD_SIZE 512 

void shell_loop() {

  char cwd[MAX_CWD_SIZE]; // for current working directory
  char prompt[MAX_PROMPT_SIZE];
  char* buffer;
  char** args;
  int status = 1;
  
  signal(SIGINT, SIG_IGN); // just ignore Ctrl-C
  
  clear();
  
  do {
    if(!getcwd(cwd, sizeof(cwd))) {
      fprintf(stderr, "shell: cannot get current working directory\n");
      break;
    }
    int len = snprintf(prompt, sizeof(prompt), "%s%s%s:%s%s%s$ ", 
                       "\x1b[1;36m", getenv("USER"), "\x1b[0m",
                       "\x1b[1;34m", cwd, "\x1b[0m");
    prompt[len] = '\0';

    buffer = readline(prompt);
  
    if(strlen(buffer) == 0)
      continue;
    
    add_history(buffer);
    
    args = shell_split_line(buffer);
    status = shell_execute(args);
    
    free(buffer);
    free(args);
    
  } while(status);
}

int main(int argc, char** argv) {
  
  shell_loop();
  
  return 0; // EXIT_SUCCESS
}
