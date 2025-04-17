/* $begin shellmain */
#include "csapp.h"
#include "myshell.h"
#include <errno.h>
#define MAXARGS 128

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv); 

int main() 
{
    char cmdline[MAXLINE]; /* Command line */

    /* signal 설정 */
    Signal(SIGCHLD, sigchld_handler);
    Signal(SIGINT, sigint_handler);
    Signal(SIGTSTP, sigtstp_handler);

    Sigemptyset(&mask);
    Sigaddset(&mask, SIGINT);
    Sigaddset(&mask, SIGTSTP);
    Sigprocmask(SIG_BLOCK, &mask, &prev);

    init_jobs();

    while (1) {
        /* Read */
        printf("CSE4100-SP-P2> ");
        if(fgets(cmdline, MAXLINE, stdin)==NULL)
        { 
        if (feof(stdin))
            exit(0);
        else 
            unix_error("fgets error");
        }
        /* Evaluate */
        eval(cmdline);
    } 
}
/* $end shellmain */

/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline) 
{
    int single_quote_cnt = 0;
    int double_quote_cnt = 0;
    int bt_cnt = 0;
    int single_quote_idx[MAXARGS];
    int double_quote_idx[MAXARGS];
    int bt_idx[MAXARGS];
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */

    /*-------------따옴표, 백틱 제거----------------*/
    for (int i = 0; i < strlen(cmdline); i++) {
        if (cmdline[i] == '|')
            pipe_flag = 1;
        else if (cmdline[i] == '\'') 
            single_quote_idx[single_quote_cnt++] = i;
        else if (cmdline[i] == '\"') 
            double_quote_idx[double_quote_cnt++] = i;
        else if (cmdline[i] == '`')
            bt_idx[bt_cnt++] = i;
    }

    if ((single_quote_cnt % 2) == 0 && single_quote_cnt != 0) {
        for (int i = 0; i < single_quote_cnt; i++) {
            cmdline[single_quote_idx[i]] = ' ';
        }
    }

    if ((double_quote_cnt % 2) == 0 && double_quote_cnt != 0) {
        for (int i = 0; i < double_quote_cnt; i++) {
            cmdline[double_quote_idx[i]] = ' ';
        }
    }

    if ((bt_cnt % 2) == 0 && bt_cnt != 0) {
        for (int i = 0; i < bt_cnt; i++) {
            cmdline[bt_idx[i]] = ' ';
        }
    }
    /*-------------따옴표, 백틱 제거----------------*/

    /*-------------백틱 파이프화-------------------*/
    if ((bt_cnt % 2) == 0 && bt_cnt != 0) {
        pipe_flag = 1; // echo `ls -al` `ls` -> echo | ls -al | ls 동일한 결과

        for (int i = 0; i < bt_cnt; i += 2) {
            cmdline[bt_idx[i]] = '|';

            for (int j = bt_idx[i + 1] - 1; j > bt_idx[i]; j--) {  
                cmdline[j + 1] = cmdline[j];
            }

            cmdline[bt_idx[i] + 1] = ' ';  
        }
    }
    /*----------------백틱 파이프화-------------------*/

    strcpy(buf, cmdline);
    bg = parseline(buf, argv);

    if (argv[0] == NULL)  
        return;   /* Ignore empty lines */

    if (!builtin_command(argv)) { // quit -> exit(0), & -> ignore, other -> run
        char path[50] = "/bin/";
        strcat(path, argv[0]);

        if ((pid = Fork()) == 0) {
            setpgid(0, 0);
            sigprocmask(SIG_UNBLOCK, &mask, NULL);

            if (pipe_flag) {
                char *parent_cmd[50] = {NULL};
                char *child_cmd[50] = {NULL};

                splitpipe(argv, parent_cmd, child_cmd);
                excutepipe(parent_cmd, child_cmd);
            } else if (execve(path, argv, environ) < 0) {    
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
        }
        /* Parent waits for foreground job to terminate */
        else {
            setpgid(pid, pid);
            if(!bg)
                add_job(pid,'F', cmdline);
            else
                add_job(pid,'B', cmdline);

            sigprocmask(SIG_UNBLOCK, &mask, NULL);

            if (!bg) { 
                int status;
                Waitpid(pid, &status, 0);

                if (WIFEXITED(status) || WIFSIGNALED(status)) {
                    job_t *job = find_job_by_pid(pid);
                    if(job)
                        delete_job(pid);
                }
                else if(WIFSTOPPED(status)){
                    job_t *job = find_job_by_pid(pid);
                    if(job)
                        job->state = 'S';
                }
            } else {    //background 일때
                job_t *job = find_job_by_pid(pid);
                if (job) printf("[%d] (%d) %s", job->jid, pid, cmdline);
            }
        }
    }
    return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv)
{
    if (!strcmp(argv[0], "quit") || !strcmp(argv[0], "exit")) exit(0);
    if (!strcmp(argv[0], "&")) return 1;

    if (!strcmp(argv[0], "cd")) {
        char *d_path = (argv[1] == NULL) ? getenv("HOME") : argv[1];
        if (chdir(d_path) != 0) printf("Error : invalid path\n");
        return 1;
    }

    if (!strcmp(argv[0], "jobs")) {
        list_jobs();
        return 1;
    }

    if (!strcmp(argv[0], "bg")) {
        job_t *job = NULL;
        if (argv[1][0] == '%') job = find_job_by_jid(atoi(&argv[1][1]));
        else if (isdigit(argv[1][0])) job = find_job_by_pid(atoi(argv[1]));

        if (job && job->state == 'S') {
            job->state = 'B';
            Kill(-(job->pid), SIGCONT);
        }
        return 1;
    }

    if (!strcmp(argv[0], "fg")) {
        job_t *job = NULL;
        if (argv[1][0] == '%') job = find_job_by_jid(atoi(&argv[1][1]));
        else if (isdigit(argv[1][0])) job = find_job_by_pid(atoi(argv[1]));

        if (job) {
            job->state = 'F';
            Kill(-(job->pid), SIGCONT);

            sigprocmask(SIG_UNBLOCK, &prev, NULL);

            int status;
            Waitpid(job->pid, &status, WUNTRACED);

            if (WIFSTOPPED(status)) job->state = 'S';
            else delete_job(job->pid);
        }
        return 1;
    }

    if (!strcmp(argv[0], "kill")) {
        job_t *job = NULL;
        if (argv[1][0] == '%') job = find_job_by_jid(atoi(&argv[1][1]));
        else if (isdigit(argv[1][0])) job = find_job_by_pid(atoi(argv[1]));

        if (job) {
            Kill(job->pid, SIGKILL);
            job->state = 'T';
        }
        return 1;
    }

    return 0;
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

void splitpipe(char **argv, char *parent_cmd[], char *child_cmd[]) {
    int pipe_idx = -1;

    for (int i = 0; argv[i] != NULL; i++) {
        if (!strcmp(argv[i], "|")) {
            pipe_idx = i + 1;
            break;
        }
        parent_cmd[i] = argv[i];
    }
    
    parent_cmd[pipe_idx - 1] = NULL;

    if (pipe_idx != -1) {
        for (int i = 0; argv[pipe_idx + i] != NULL; i++) {
            child_cmd[i] = argv[pipe_idx + i];
        }

        child_cmd[pipe_idx] = NULL;
    }
}


void excutepipe(char *parent_cmd[], char *child_cmd[]) {
    int fd[2];
    char path[100] = "/bin/";

    if (pipe(fd) == -1) {
        printf("Create pipe error\n");
        exit(1);
    }

    if (Fork() == 0) {
        Dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);

        strcpy(path, "/bin/");
        strcat(path, parent_cmd[0]);
        if (execve(path, parent_cmd, environ) == -1) {
            printf("First execve error\n");
            exit(1);
        }
    }

    else {
        char *temp1[50] = {NULL};
        char *temp2[50] = {NULL};

        Dup2(fd[0], STDIN_FILENO);
        close(fd[1]);

        splitpipe(child_cmd, temp1, temp2);

        if (temp2[0] == NULL) {
            strcpy(path, "/bin/");
            strcat(path, temp1[0]);
            if (execve(path, temp1, environ) == -1) {
                printf("Second execve error\n");
                exit(1);
            }
        } else {
            excutepipe(temp1, temp2);
        }
    }
}


void sigchld_handler(int sig) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        job_t *job = find_job_by_pid(pid);
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            
            if (job && job->state != 'T') delete_job(pid);
        }
        else if (WIFSTOPPED(status)) {  // 프로세스가 STOPPED 상태로 변경된 경우
            if (job) {
                job->state = 'S';  // Stopped 상태로 변경
            }
        }
    }
}

void sigint_handler(int sig) {
    pid_t fg_pid = find_fg();
    if (fg_pid != -1) Kill(-fg_pid, SIGINT);
}

void sigtstp_handler(int sig) {
    pid_t fg_pid = find_fg();  // 현재 포그라운드 프로세스 ID 확인
    if (fg_pid != -1) {
        job_t *job = find_job_by_pid(fg_pid);  // 해당 PID로 job 찾기
        if (job) {
            Kill(-fg_pid, SIGTSTP);  // 프로세스를 SIGTSTP로 중지
        }
    }
}

void list_jobs()
{
    for (int i = 0; i < MAXJOBS; i++) {
        if (job_list[i].jid != -1) {
            if (job_list[i].state == 'B') {
                printf("[%d] (%d) Running %s", job_list[i].jid, job_list[i].pid, job_list[i].cmdline);
            } else if (job_list[i].state == 'S') {
                printf("[%d] (%d) Stopped %s", job_list[i].jid, job_list[i].pid, job_list[i].cmdline);
            } else if (job_list[i].state == 'T') {
                printf("[%d] (%d) Terminated %s", job_list[i].jid, job_list[i].pid, job_list[i].cmdline);
            }
        }
    }
}

void change_state(char **argv, char flag) {
    job_t *cur = NULL;

    if (argv[1] != NULL) {
        if (argv[1][0] == '%') {
            int jid = atoi(&argv[1][1]);
            cur = find_job_by_jid(jid);
        } else if (isdigit(argv[1][0])) {
            int pid = atoi(argv[1]);
            cur = find_job_by_pid(pid);
        }
    }

    if (cur == NULL) return;

    if (flag == 'B') {
        change_job_state(cur, 'B');
    } else if (flag == 'F') {
        change_job_state(cur, 'F');
    }
}

void change_job_state(job_t *cur, char state) {
    cur->state = state;  
    Kill(-(cur->pid), SIGCONT);  
}

pid_t find_fg()
{   
    for (int i = 0; i < MAXJOBS; i++) {
        if (job_list[i].state == 'F') {  // 포그라운드 또는 Stopped 상태의 작업
            return job_list[i].pid;
        }
    }
    return -1;  // 포그라운드 작업이 없을 경우
}

void init_jobs()
{   
    job_id = 1;
    for (int i = 0; i < MAXJOBS; i++) {  // job 목록 초기화
        job_list[i].jid = -1;
        job_list[i].state = -1;
        job_list[i].cmdline[0] = '\n';
        job_list[i].pid = -1;
    }
}

void add_job(pid_t pid, char state, char *cmdline)
{
    if (pid <= 0)
        return;
    
    for (int i = 0; i < MAXJOBS; i++) {
        if (job_list[i].jid == -1) {  // 빈 슬롯을 찾으면
            job_list[i].jid = job_id++;  // 새로운 작업 ID 할당
            if (job_id > MAXJOBS) {  // MAXJOBS를 초과하면 다시 1로 초기화
                job_id = 1;
            }
            job_list[i].state = state;
            strcpy(job_list[i].cmdline, cmdline);
            job_list[i].pid = pid;
            break;
        }
    }
}

void delete_job(pid_t pid)
{
    if (pid <= 0)
        return;

    for (int i = 0; i < MAXJOBS; i++) {
        if (job_list[i].pid == pid) {
            printf("delete add job");
            job_list[i].jid = -1;
            job_list[i].state = -1;
            job_list[i].cmdline[0] = '\n';
            job_list[i].pid = -1;
            break;
        }
    }
}

job_t *find_job_by_jid(int jid)
{
    if (jid <= 0)
        return NULL;
    
    for (int i = 0; i < MAXJOBS; i++) {
        if (job_list[i].jid == jid)
            return &job_list[i];
    }

    return NULL;
}

job_t *find_job_by_pid(pid_t pid)
{
    if (pid <= 0)
        return NULL;
    
    for (int i = 0; i < MAXJOBS; i++) {
        if (job_list[i].pid == pid)
            return &job_list[i];
    }

    return NULL;
}