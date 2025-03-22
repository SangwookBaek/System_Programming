/* $begin shellmain */
#include "csapp.h"
#include<errno.h>
#include <ctype.h>


#define MAXARGS   128

typedef struct _node {
    int id;
    char *cmd;
    struct _node* next;
    struct _node* prev;
}node;


/* Function prototypes */
void eval(char *cmdline, node*, node*,node*, int);
int parseline(char *buf, char **argv);
int builtin_command(char **argv,node*, node*, node*, int); 



node* cur = NULL;
node* tail = NULL;
node* head;
char cwd[PATH_MAX];

int line_len;

//new funcs
int addnode(node*, int ,char*);
void printhist(node*); //linkedlist에 있는 hist를 한번에 쫙 출력하는 함수 head만 전해주면 내부 pointer새로 만들어서 굴린다
int check_mark(char* ); //지금 입력받은애가 !! or !#인지 체크하는 함수 return에 따라 달라진다
int memory_free(node* ,node*, node*); //exit 시점에 memory free하기
int writehistory(node*,FILE*);//hist를 쫙 파일에 입력해주는 친구

int main() 
{   
    getcwd(cwd, sizeof(cwd));
    FILE *fp;
    head = (node*)malloc(sizeof(node));
    head->next = NULL;
    head->prev = NULL;
    head->id = 0;
    node* cur = head;
    node* tail = head;
    int mark;


    char line[MAXLINE];
    char cmdline[MAXLINE]; /* Command line */
    fp = fopen( "history.txt", "r" );
    if (!fp){
        fp = fopen("history.txt", "w");
        fclose(fp);
        fp = fopen( "history.txt", "r" );
    }
    
    while (fgets(line, MAXLINE, fp)){
        addnode(cur,strlen(line),line);
        cur = cur->next;
    }
    fclose(fp);

    tail = cur; //tail이 마지막을 link하도록 설정
    //printhist(head); 
 
    while (1) {
	/* Read */
	printf("> ");                   
	fgets(cmdline, MAXLINE, stdin);
    mark = check_mark(cmdline);

	if (feof(stdin))
	    exit(0);
	/* Evaluate */

    // if ((mark==0)&&(!cmdline)){
    //     addnode(cur,strlen(cmdline),cmdline);
    //     cur = cur->next;
    //     tail = cur;
    // }
    // else{
    //     printf("!! or !# or null");
    // }
	eval(cmdline,head,tail,cur,mark);
    }
}
/* $end shellmain */
  
/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline,node* head, node* tail,node* cur,int mark) 
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */
    
    strcpy(buf, cmdline);
    bg = parseline(buf, argv); 

    if (argv[0] == NULL)
	return;   /* Ignore empty lines */
    if (mark ==0){
        addnode(cur,strlen(cmdline),cmdline);
        cur = cur->next;
        tail = cur;
    }

    if (!builtin_command(argv,head,tail,cur,mark)) { //quit -> exit(0), & -> ignore, other -> run
        if ((pid= fork())==0){
            char temp[MAXLINE] = "/bin/";
            strncat(temp, argv[0], strlen(argv[0]));
            if (execve(temp, argv, environ) < 0) {	//ex) /bin/ls ls -al &
            printf("%s: Command not found.\n", argv[0]);
            exit(0);
            }
        }
	/* Parent waits for foreground job to terminate */
	if (!bg){ 
	    int status;
        if (waitpid(pid,&status,0) < 0) unix_error("waitfg : waitpid error");
	}
	else//when there is backgrount process!
	    printf("%d %s", pid, cmdline);
    }
    return;
}



/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv,node* head, node* tail,node* cur,int mark) 
{
    FILE* fp;
    node *tmp; //직접 head랑 tail을 옮기면 안된다 tmp로 움직여야지 위치가 고정
    int i=0;
    if (!strcmp(argv[0], "quit")||!strcmp(argv[0], "exit")){ /* quit command */
        chdir(cwd);
        fp = fopen("history.txt", "w");
        writehistory(head,fp);
        fclose(fp);
        memory_free(cur,head,tail);
	    exit(0);
    }
    if (!strcmp(argv[0], "&"))    /* Ignore singleton & */
	return 1;

    if (!strcmp(argv[0],"cd")){ //이부분을 구현해야함
        int status;
        if(argv[1] == NULL || !strcmp(argv[1], "~") || !strcmp(argv[1], "$HOME"))
            status = chdir(getenv("HOME"));
        else
            status = chdir(argv[1]);
        return 1;
    }
    if (!strcmp(argv[0],"history")){ //이부분을 구현해야함
        printhist(head); //linkedlist의 목록을 모두 출력
        return 1;
    } 
    if (mark == -1){ //!!
        printf("%s",tail->cmd); //일단 마지막 cmd를 출력
        mark = check_mark(tail->cmd);
        eval(tail->cmd,head,tail,cur,mark); //그 cmd를 다시 eval에 넣어줌
        return 1;
    } 
    if (mark>0){ //!#
        tmp = tail;
        while(tmp->id != mark){
            tmp = tmp->prev;
        }
        printf("%s",tmp->cmd);
        eval(tmp->cmd,head,tail,cur,0);
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
    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' '))) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* Ignore blank line */
	return 1;

    /* Should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0)
	argv[--argc] = NULL;

    return bg;
}
/* $end parseline */


int addnode(node* cur, int line_len, char* line){
    char *cmd = (char*)malloc(sizeof(char)*(line_len+1));
    strncpy(cmd,line,line_len+1);
    node* tmp = (node*)malloc(sizeof(node)); //tmp가 추가할 애
    tmp->cmd = cmd;
    tmp->next = NULL;
    cur->next = tmp; 
    tmp->prev = cur;
    tmp->id = tmp->prev->id + 1;
}

void printhist(node* head){ //head 주소주면 head부터 끝까지 쭉 읽는다
    node* cur; 
    cur = head->next;
    while (cur){
        printf("%d  %s",cur->id,cur->cmd);
        cur = cur->next;
    }
}

int check_mark(char* cmdline){ //0이면 !으로 시작안함 or !+문자, -1이면 !!임, 숫자면 !+숫자
    int i=1;
    int k=1;
    int m = 0;
    int digit = 0;
    int hist_number  = 0;
    int cmd_len=0;
    cmd_len = strlen(cmdline);
    if ((cmdline[0] == '!') && (cmdline[1]=='!')){
        if ((cmd_len>=3)&&(cmdline[2]=='!')) {
            return 0;
        }
        return -1;
    }
    if (cmdline[0]=='!') {
        hist_number=atoi(&cmdline[1]);
        for (m=1;m<cmd_len-1;m++){
            if (!isdigit(cmdline[m])){
                return 0;
            }
        }
        return hist_number;
    }
    return 0;
}


int memory_free(node* cur,node* head, node* tail){
    node* tmp; 
    cur = head;
    while (cur){
        tmp = cur;
        free(tmp); 
        cur = cur->next;
    }
    return 1;
}


int writehistory(node* head,FILE* fp){
    node* tmp; 
    cur = head->next;
    while (cur->next){
        tmp = cur;
        fprintf(fp,"%s",tmp->cmd);
        cur = cur->next;
    }
    return 1;
}