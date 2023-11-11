#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <glob.h>

#define DELIMS " \t\n"

struct cmd_st {
    glob_t globres;
};

static void prompt(void) {
    printf("mysh$ ");
}

static void parse(char *line, struct cmd_st *res) {
    char *tok;
    int i = 0;
    while(1) {
        // strsep 函数将缓冲区按换行符分割
        tok = strsep(&line, DELIMS);
        if(tok == NULL)
            break;
        if(tok[0] == '\0')
            continue;
       	// 利用 glob 函数的参数特点来模拟 argc 和 argv
        // NOCHECK：不对pattern进行解析，直接返回pattern（这里是tok），相当于存储了命令行参数tok在glob_t中
        // APPEND：以追加形式将tok存放在glob_t中，第一次时不追加，因为globres尚未初始化，需要系统来自己分配内存，因此乘上i（乘法优先于按位或）
        glob(tok, GLOB_NOCHECK|GLOB_APPEND*i, NULL, &res->globres);
        // 置为1，使得追加永远成立
        i = 1;
    }
}

int main(void) {
    char *linebuf = NULL;
    size_t linebuf_size = 0;
    struct cmd_st cmd;
    pid_t pid;
    while(1) {
        prompt();
        if(getline(&linebuf, &linebuf_size, stdin) < 0) {
            break;
        }

        parse(linebuf, &cmd);
        if(strcmp(cmd.globres.gl_pathv[0], "exit") == 0) {
            exit(0);
        }
        if(0) {
            // 内部命令
        }
        else {
            pid = fork();
            if(pid < 0) {
                perror("fork()");
                exit(1);
            }
            if(pid == 0) {
                execvp(cmd.globres.gl_pathv[0], cmd.globres.gl_pathv);
                perror("execvp()");
                exit(1);
            } else {
                wait(NULL);
            }
        }
    }
    exit(0);
}