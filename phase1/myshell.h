/* Function prototypes */
#define MAXARGS   128
#define MAXCMDS 10

#include "csapp.h"

unsigned int pipe_flag; /*pipe 사용을 하는지 구분*/
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv); 
char ** split_command(char *cmdline); 
