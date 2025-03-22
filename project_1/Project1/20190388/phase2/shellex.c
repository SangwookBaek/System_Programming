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
void eval(char *cmdline,int);
int parseline(char *buf, char **argv);
int builtin_command(char **argv,int); 
int parsepipe(char*, char*);


node* cur = NULL;
node* tail = NULL;
node* head;
char cwd[PATH_MAX];

int line_len;
int new_file_flag = 0;//history를 처음만들어서 열었는지 확인하는 flag
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
    cur = head;
    tail = head;
    int mark;


    char line[MAXLINE];
    char cmdline[MAXLINE]; /* Command line */
    fp = fopen( "history.txt", "r" );
    if (!fp){
        new_file_flag = 1;
        fp = fopen("history.txt", "w");
        fclose(fp);
        fp = fopen( "history.txt", "r" );
    }
    int history_count = 0;
    while (fgets(line, MAXLINE, fp)){
        addnode(cur,strlen(line),line);
        history_count ++;
        cur = cur->next;
    }
    if (history_count ==0){
        new_file_flag = 1;
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
	eval(cmdline,mark);
    }
}
/* $end shellmain */
  
/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline,int mark) 
{
    //char *argv_pipe[5][MAXLINE];
    char ***argv_pipe;
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    char pipe_buf[MAXLINE]; //pipe를 처리한 Cmdline을 집어넣을 buffer
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */
    char *cmds[MAXARGS];
    int pipe_num;
    int tmp_len; //pipe input이 되는 cmd의 len
    char tmp_cmd[MAXLINE]; // pipe의 input이 되는 cmd
    char *tmp_pointer; //pipe input의 첫 원소 가리키는 pointer
    int pipe_i;
    int pipe_success = 0;
    int *fd;//pipe 함수를 사용할 fd 선언
    strcpy(buf, cmdline);
    memset(&pipe_buf[0], 0, sizeof(pipe_buf));
    pipe_num = parsepipe(pipe_buf,buf);
    fd = (int*)malloc(sizeof(int)*(pipe_num+1)*2);//fd에 pipe_num*2개만큼 메모 할당
    if (mark ==0){
        if (new_file_flag){
            addnode(cur,strlen(cmdline),cmdline);
            cur = cur->next;
            tail = cur;
            new_file_flag = 0;
        }
        else{
            if ((!strcmp(cur->cmd,"history\n"))&&(!strcmp(cmdline,"history\n"))){

            }
            else{
                addnode(cur,strlen(cmdline),cmdline);
                cur = cur->next;
                tail = cur;
            }
        }
        
    }
    if (pipe_num){ //pipe가 있는경우
        tmp_pointer = pipe_buf;
        for (pipe_i = 0;pipe_i<=pipe_num;pipe_i++){
            if(pipe(fd + pipe_i*2)){ //미리 pipe를 만들어둔다.
                printf("pipe error\n");
                exit(1);
            }
        }
        int tmp_fd = 0;
        for (pipe_i = 0;pipe_i<=pipe_num;pipe_i++){ //파이프수만큼 cmd 나눠서 반복
            tmp_len = strlen(tmp_pointer);
            strncpy(tmp_cmd,tmp_pointer,tmp_len);
            bg = parseline(tmp_cmd, argv); //pipe의 입력을 parseline에 집어넣는다
            tmp_pointer = tmp_pointer + (tmp_len+1); //pipe_buf를 pipe 뒤로 넘긴다.
            if (argv[0] == NULL)
            return;   /* Ignore empty lines */

            if ((pid = fork())==0){ //child
                if (pipe_i != pipe_num){//파이프의 마지막이 아닌 경우
                    if(dup2(fd[tmp_fd+1],1)<0){ //쓰는쪽을 1로해
                        perror("dup2");
                        exit(0);
                    }
                }
                if (tmp_fd !=0){
                    if (dup2(fd[tmp_fd-2],0)<0){ //pipe의 input의 
                        perror("dup2");
                        exit(0);
                    }
                }
                for (int m = 0;m<(2*(pipe_num+1));m++){
                    close(fd[m]);
                }
                free(fd);
                if (!builtin_command(argv,mark)){
                    if (execvp(argv[0], argv) < 0) {	//ex) /bin/ls ls -al &
                        printf("%s: Command not found.\n", argv[0]);
                        exit(0);
                    }
                }
                else{
                    exit(0);
                }
            }
            tmp_fd = tmp_fd + 2;
            memset(&tmp_cmd[0], 0, sizeof(tmp_cmd));
        }
        for(int i = 0; i < 2 * (pipe_num+1); i++){
            close(fd[i]);
        }
        free(fd);
        /* Parent waits for foreground job to terminate */
        if (!bg){ 
            int status;
            //if (waitpid(pid,&status,0) < 0) unix_error("waitfg : waitpid error");
            //for(int i = 0; i <= pipe_num; i++)
            while((pid = wait(NULL))>0);
            //if (wait(&status)<0) unix_error("waitfg : waitpid error");
            // if (wait(&status)<0) unix_error("waitfg : waitpid error");
        }
    }
    else{
        //strcpy(buf, cmdline);
        bg = parseline(buf, argv); 
        if (argv[0] == NULL)
        return;   /* Ignore empty lines */

        if (!builtin_command(argv,mark)) { //quit -> exit(0), & -> ignore, other -> run
            if ((pid= fork())==0){
                if (execvp(argv[0], argv) < 0) {	//ex) /bin/ls ls -al &
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
    }
    return;
}



/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv,int mark) 
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
        //mark = check_mark(tail->cmd);
        eval(tail->cmd,0); //그 cmd를 다시 eval에 넣어줌
        return 1;
    } 
    if (mark>0){ //!#
        tmp = tail;
        while(tmp->id != mark){
            tmp = tmp->prev;
        }
        printf("%s",tmp->cmd);
        eval(tmp->cmd,0);
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

    //buf는 cmdline을 copy한것이다
    buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
	buf++;
    //buf의 endlinew제거한 부분
    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' '))) { //빈칸이 있는 부분 마다 자른다 delim이 빈칸으로 나눠지는 곳 pointer
    //buf는 빈칸 이후부터의 공간에 대한 char array
	argv[argc++] = buf; 
	*delim = '\0'; //공백을 \0으로 처리한다 delimd이 빈칸으로 나눠지는 지점인데 거길 \0할당이니까.
	buf = delim + 1; //공백한칸 뒤로 buf를 옮겨
	while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;
    }
    argv[argc] = NULL;
    int i =0;
    if (argc == 0)  /* Ignore blank line */
	return 1;

    /* Should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0)
	argv[--argc] = NULL;
    return bg;
}
/* $end parseline */

int parsepipe(char *pipe_buffer,char* buf){
    int double_state = 0; // " 를 만나면 state를 1로 만들어둔다 "를 다시 만나면 0으로 바꿈
    int single_state = 0; //  '를 만나면 state를 1로 만들어둔다 '를 다시 만나면 0으로 바꿈
    int j=0;
    int pipe_num = 0;
    for (int i =0;i<strlen(buf);i++){ //endline 처리는냅둬 어차피 parseline에서 해준다
        if ((buf[i]=='\"') && double_state && !single_state ){ // "를 두번째 마주침
            double_state = 0;
        }
        else if ((buf[i]=='\"') && !double_state && !single_state ){
            double_state = 1;
        }
        else if ((buf[i]=='\'') && !single_state && !double_state ){
            single_state = 1;
        }
        else if ((buf[i]=='\'') && single_state && !double_state ){
            single_state = 0;
        }
        else if ((buf[i]=='|')&&!single_state&&!double_state) {
            pipe_buffer[j++] = '\n';
            pipe_buffer[j++] = '\0';
            pipe_num++;
        }
        else{
            pipe_buffer[j] = buf[i];
            j++;
        }
    }
    return pipe_num;
}



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
