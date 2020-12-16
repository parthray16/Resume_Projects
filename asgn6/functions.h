#ifndef functions
#define functions

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>

#define CMDLINELENGTH 512
#define CMDPIPELIM 10
#define ARGSLIM 10
#define READ 0
#define WRITE 1

int readline(FILE *file, char *line);
int exec_stage(char *line, int stage, int in, int out,
             int totalPipes, int prev[], int next[]);
#endif
