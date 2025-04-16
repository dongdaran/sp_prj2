/* Function prototypes */
#define MAXARGS   128
#define MAXCMDS 10
#define MAXJOBS 50

#include "csapp.h"

unsigned int pipe_flag; /*pipe 사용을 하는지 구분*/
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv); 

/* phase2 */
void splitpipe(char **argv, char *parent_cmd[], char *child_cmd[]);
void excutepipe(char *parent_cmd[], char *child_cmd[]);
/* phase2 */

/* phase3 추가 */

typedef struct job_t {
    pid_t pid;              /* Process ID */
    int jid;                /* Job idx */
    int state;              /* Job state: BG, FG, or ST */
    char cmdline[MAXLINE];  /* Command line */
} job_t;

int job_id;      /* job에 부여될 id */
job_t job_list[MAXJOBS]; /* job의 목록 */


/* signal 처리 */
sigset_t mask, prev;
void sigchld_handler(int sig);
void sigint_handler(int sig);
void sigtstp_handler(int sig);
/* signal 처리 */

void init_jobs();
void list_jobs();
void change_state(char **argv, char flag);
void change_job_state(job_t *cur, char state);
pid_t find_fg();

void add_job(pid_t pid, char state, char *cmdline);
void delete_job(pid_t pid);

job_t *find_job_by_jid(int jid);
job_t *find_job_by_pid(pid_t pid);

pid_t shell_pgid;
/* job 관리*/

/* phase3 추가 */