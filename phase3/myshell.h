/* Function prototypes */
#define MAXARGS   128
#define MAXCMDS 10


void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv); 
char ** split_command(char *cmdline); 