/* $begin shellmain */
#include "csapp.h"
#include<errno.h>
#define MAXARGS   128

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv); 

int main() 
{
    char cmdline[MAXLINE]; /* Command line */

    while (1) {
	/* Read */
	printf("CSE4100-SP-P2> ");                   
	fgets(cmdline, MAXLINE, stdin); 
	if (feof(stdin))
	    exit(0);

	/* Evaluate */
	eval(cmdline);
    } 
}
/* $end shellmain */
  
/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline) 

{   int single_quote_cnt = 0;
    int double_quote_cnt = 0;
    int bt_cnt = 0;
    int argc;
    int single_quote_idx[MAXARGS];
    int double_quote_idx[MAXARGS];
    int bt_idx[MAXARGS];
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */
    
    /*-------------따옴표, 백틱 제거----------------*/
    for (int i = 0; i < strlen(cmdline); i++) {
    if (cmdline[i] == '\'') 
      single_quote_idx[single_quote_cnt++] = i;
    else if (cmdline[i] == '\"') 
      double_quote_idx[double_quote_cnt++] = i;
    else if (cmdline[i] == '`')
        bt_idx[bt_cnt++] = i;
    }

    if((single_quote_cnt % 2) == 0 &&single_quote_cnt != 0) {
    for (int i = 0; i < single_quote_cnt; i++) {
      cmdline[single_quote_idx[i]] = ' ';
        }
    }

    if((double_quote_cnt % 2) == 0 && double_quote_cnt != 0) {
    for (int i = 0; i < double_quote_cnt; i++) {
      cmdline[double_quote_idx[i]] = ' ';
        }
    }

    if((bt_cnt % 2) == 0 && bt_cnt != 0) {
    for (int i = 0; i < bt_cnt; i++) {
      cmdline[bt_idx[i]] = ' ';
    }
  }
  /*-------------따옴표, 백틱 제거----------------*/

    strcpy(buf, cmdline);
    bg = parseline(buf, argv);
    
    /*argv의 인자 구하기*/
    while (argv[argc] != NULL) {
    argc++;
    }

    if (!strcmp(argv[0], "echo") && (bt_cnt % 2) == 0) 
    {
        strcpy(argv[0], argv[1]);

        for(int i = 1; i < argc; i++)
            argv[i] = NULL;
    }

    if (argv[0] == NULL)  
	return;   /* Ignore empty lines */
    if (!builtin_command(argv)) { //quit -> exit(0), & -> ignore, other -> run
        char b_path[50] = "/bin/";
        strcat(b_path, argv[0]);

        if((pid =Fork())==0){
            if (execve(b_path, argv, environ) < 0) {	//ex) /bin/ls ls -al & 
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
        }
        }
	/* Parent waits for foreground job to terminate */
    else{
	if (!bg){ 
	    int status;
        Waitpid(pid, &status, 0);
    }
	else//when there is background process!
	    printf("%d %s", pid, cmdline);
        }
    }
    return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv) 
{
    if (!strcmp(argv[0], "quit")) /* quit command */
	    exit(0);  
    if (!strcmp(argv[0], "exit")) /* exit command */
	    exit(0);  
        
    if (!strcmp(argv[0], "&"))    /* Ignore singleton & */
	    return 1;
    if (!strcmp(argv[0], "cd"))
    {   
        char *d_path;
        if(argv[1] == NULL)
            d_path = (char*)getenv("HOME");
        else if(!strcmp(argv[1], ".."))
            d_path = "..";
        else
            d_path = argv[1];

        if(chdir(d_path) != 0)
            printf("Error : invalid path");
        return 1;
    }

    return 0;                     /* Not a builtin command */
}
/* $end eval */

/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv) 
{
    char *delim;         /* Points to first space delimiter */
    int argc;            /* Number of args */
    int bg;              /* Background job? */

    buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* Ignore leading space -> 앞에 있는 공백 삭제 */
	    buf++;

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' '))) {
        argv[argc++] = buf;
        *delim = '\0'; // 문자열 나누기 
        buf = delim + 1;
        while (*buf && (*buf == ' ')) /* Ignore spaces */
                buf++;
    }
    argv[argc] = NULL; // 끝에 NULL 설정
    
    if (argc == 0)  /* Ignore blank line */
	return 1;

    /* Should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0)
	argv[--argc] = NULL;

    return bg;
}
/* $end parseline */