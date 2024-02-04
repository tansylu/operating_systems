#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#include <pthread.h>
#define MAX_THREADS 100
typedef struct {
  pthread_t thread_id;
  pid_t child_pid;
  int * fd; 
}
BackgroundInfo;
pthread_mutex_t lock;
BackgroundInfo background_jobs[MAX_THREADS];
int background_job_count = 0;
// Function declarations
int executeForeground(char * in_file, char * out_file, char cmd_path[], char * argv[]);
int executeBackground(char * in_file, char * out_file, char cmd_path[], char * argv[]);
void * readPipe(void * arg);
void parse_line(char * line, char * argv[], char ** in_file, char ** out_file, bool * background);
int main() {
  if (pthread_mutex_init( & lock, NULL) != 0) {
    printf("\n mutex init has failed\n");
    return 1;
  }
  FILE * fp;
  char * line = NULL;
  char * out_file = NULL;
  char * in_file = NULL;
  bool background = false;
  size_t len = 0;
  ssize_t read;
  fp = fopen("commands.txt", "r");
  if (fp == NULL) {
    perror("Error opening commands.txt");
    return -1;
  }
  while ((read = getline( & line, & len, fp)) != -1) {
    background = false;
    char * argv[5];
    for (int i = 0; i < 5; i++) {
      argv[i] = strdup("none");
    }
    in_file = "none";
    out_file = "none";
    char cmd_path[50] = "/usr/bin/";
    char cmd_name[15] = "none";
    parse_line(line, argv, & in_file, & out_file, & background);
    strcpy(cmd_name, argv[0]);
    strcat(cmd_path, cmd_name);
    if (strcmp(argv[0], "wait") == 0) {
      //printf("here!\n");
      for (int i = 0; i < background_job_count; i++) {
        int status;
        waitpid(background_jobs[i].child_pid, & status, 0);
        if (pthread_equal(background_jobs[i].thread_id, pthread_self()) == 0){ pthread_join(background_jobs[i].thread_id, NULL);}
        close(background_jobs[i].fd[0]);
        free(background_jobs[i].fd);
      }
      background_job_count = 0;
    } else {
      //printf("\n%s\n", argv[0]);
  if (background) {
    if (executeBackground(in_file, out_file, cmd_path, argv) != 0) {
      fprintf(stderr, "Error executing background command: %s\n", line);
    }}
      else {
    if (executeForeground(in_file, out_file, cmd_path, argv) != 0) {
      fprintf(stderr, "Error executing command: %s\n", line);
    }}
    }
  }
  fclose(fp);
  if (line)
    free(line);
  for (int i = 0; i < background_job_count; i++) {
    int status;
    waitpid(background_jobs[i].child_pid, & status, 0);
    if (pthread_equal(background_jobs[i].thread_id, pthread_self()) == 0) {
      pthread_join(background_jobs[i].thread_id, NULL);}
      close(background_jobs[i].fd[0]);
      free(background_jobs[i].fd);
    }
    pthread_mutex_destroy( & lock);
    return 0;
}


int executeForeground(char * in_file, char * out_file, char cmd_path[], char * argv[]) {
    int * fd = malloc(2 * sizeof(int));
    if (pipe(fd) == -1) {
      perror("Pipe Failed");
      exit(EXIT_FAILURE);
    }
    pthread_t thread_id;
    // Only create a thread if output redirection is not specified
    if (strcmp(argv[3], ">") != 0) {
      pthread_create( & thread_id, NULL, readPipe, & fd[0]);
    } else {
      // If output redirection is specified, set thread_id to NULL
      thread_id = pthread_self();
    }
    int rc = fork();
    if (rc == -1) {
      perror("fork");
      exit(EXIT_FAILURE);
    } else if (rc == 0) {
      close(fd[0]);
      int last = 0;
      char * res[4];
      char *
        const envp[] = {
          NULL
        };
      for (int i = 0; i < 3; i++) {
        if (strcmp(argv[i], "none") != 0) {
          res[last] = argv[i];
          last++;
        }
      }
      res[last] = NULL;
      if (strcmp(argv[3], "<") == 0) {
        int input_fd = open(in_file, O_RDONLY);
        if (input_fd == -1) {
          perror("open");
          exit(EXIT_FAILURE);
        }
        dup2(input_fd, STDIN_FILENO);
        close(input_fd);
      }
      if (strcmp(out_file, "none") != 0) {
    int output_fd = open(out_file, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (output_fd == -1) {
      perror("open");
      exit(EXIT_FAILURE);
    }
    dup2(output_fd, STDOUT_FILENO);
    close(output_fd);
      } else {
        dup2(fd[1], STDOUT_FILENO);
      }
      if (execve(cmd_path, res, envp) == -1) {
        perror("execve");
        exit(EXIT_FAILURE);
      }
      close(fd[1]);
    } else {
      close(fd[1]);
      int status;
      waitpid(rc, & status, 0);
      if (strcmp(argv[4], ">") != 0) {
        pthread_join(thread_id, NULL);
      }
      close(fd[0]);
      free(fd);
    }
    return 0;
}
    
    
int executeBackground(char * in_file, char * out_file, char cmd_path[], char * argv[]) {
  int * fd = malloc(2 * sizeof(int));
  if (pipe(fd) == -1) {
    perror("Pipe Failed");
    exit(EXIT_FAILURE);
  }
  pthread_t thread_id;
  if (strcmp(argv[3], ">") != 0) {
    pthread_create( & thread_id, NULL, readPipe, & fd[0]);
  } else {
    thread_id = pthread_self();
  }
  int rc = fork();
  if (rc == -1) {
    perror("fork");
    exit(EXIT_FAILURE);
  } else if (rc == 0) {
    close(fd[0]);
    int last = 0;
    char * res[4];
    char *
      const envp[] = {
        NULL
      };
    for (int i = 0; i < 3; i++) {
      if (strcmp(argv[i], "none") != 0) {
        res[last] = argv[i];
        last++;
      }
    }
    res[last] = NULL;
    if (strcmp(argv[3], "<") == 0) {
      int input_fd = open(in_file, O_RDONLY);
      if (input_fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
      }
      dup2(input_fd, STDIN_FILENO);
      close(input_fd);
    }
    if (strcmp(argv[3], ">") == 0) {
      int output_fd = open(out_file, O_WRONLY | O_CREAT | O_APPEND, 0666);
      if (output_fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
      }
      dup2(output_fd, STDOUT_FILENO);
      close(output_fd);
    } else {
      dup2(fd[1], STDOUT_FILENO);
    }
    if (execve(cmd_path, res, envp) == -1) {
      perror("execve");
      exit(EXIT_FAILURE);
    }
    close(fd[1]);
  } else {
    close(fd[1]);
    BackgroundInfo myBackgroundJob;
    myBackgroundJob.thread_id = thread_id;
    myBackgroundJob.child_pid = rc;
    myBackgroundJob.fd = fd;
    pthread_mutex_lock( & lock);
    background_jobs[background_job_count] = myBackgroundJob;
    background_job_count++;
    pthread_mutex_unlock( & lock);
  }
  return 0;
}
    
    
void *readPipe(void *arg) {
  int r1 = *((int *)arg);
  char buff[1024];
  pthread_t thread_id = pthread_self();
  FILE *pipe_stream = fdopen(r1, "r");

  if (pipe_stream == NULL) {
      perror("fdopen");
      return NULL;
  }

  pthread_mutex_lock(&lock);
  printf("\n------ Thread ID: %lu\n", thread_id);
  ssize_t bytesRead;

  while ((bytesRead = fread(buff, 1, sizeof(buff), pipe_stream)) > 0) {
      printf("%.*s", (int)bytesRead, buff);
      fflush(stdout);
  }

  printf("\n------ Thread ID: %lu\n", thread_id);
  fflush(stdout);

  pthread_mutex_unlock(&lock);

  return NULL;
}

    
    
void parse_line(char * line, char * argv[], char ** in_file, char ** out_file, bool * background) {
  int counter = 1;
  char * word;
  word = strtok(line, " ");
  FILE * parse_output = fopen("parse.txt", "a");
  fprintf(parse_output, "----------\n");
  while (word != NULL && word != "\n") {
    size_t len = strcspn(word, "\n");
    if (len > 0) {
      word[len] = '\0';
    }
    if (counter == 1) {
      fprintf(parse_output, "Command: %s\n", word);
      argv[0] = strdup(word);
    } else {
      if (word[0] == '-') {
        fprintf(parse_output, "Option: %s\n", word);
        argv[1] = strdup(word);
      } else if (strcmp(word, "<") == 0 || strcmp(word, ">") == 0) {
        fprintf(parse_output, "Redirection: %s\n", word);
        argv[3] = strdup(word);
        word = strtok(NULL, " ");
        size_t len = strcspn(word, "\n");
        if (len > 0) {
          word[len] = '\0';
        }
        if (strcmp(argv[3], "<") == 0) {
          * in_file = strdup(word);
        } else if (strcmp(argv[3], ">") == 0) {
          * out_file = strdup(word);
        }
      } else if (strcmp(word, "&") == 0) {
        * background = true;
      } else {
        fprintf(parse_output, "Inputs: %s\n", word);
        argv[2] = strdup(word);
      }
    }
    word = strtok(NULL, " ");
    counter++;
  }
  if (strcmp(argv[2], "none") == 0) {
    fprintf(parse_output, "Inputs:\n");
  }
  if (strcmp(argv[1], "none") == 0) {
    fprintf(parse_output, "Options:\n");
  }
  if (strcmp(argv[3], "none") == 0) {
    fprintf(parse_output, "Redirection: -\n");
  }
  if ( * background == false) {
    fprintf(parse_output, "Background Job: n\n");
    * background = false;
    fprintf(parse_output, "----------\n");
  } else {
    fprintf(parse_output, "Background Job: y\n");
    fprintf(parse_output, "----------\n");
  }
  fclose(parse_output);
}
