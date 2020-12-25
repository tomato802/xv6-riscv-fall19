#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"
#include "kernel/fcntl.h"

#define MAXSTR    32
#define EXEC      0
#define REDIR     1
#define PIPE      2

typedef struct cmd
{
    int argc;
    // argv的个数
    char *argv[MAXSTR];
    // argv
    int type;
    // 命令的类型 
    char input[MAXSTR];
    // 输入重定向
    char output[MAXSTR];
    // 输出重定向
    char right[MAXSTR];
    // 管道右侧指令（左侧为自身）
} cmd;


int
getcmd(char *buf, int bufsize)
{
    printf("@ ");
    memset(buf, 0, bufsize);
    gets(buf, bufsize);
    if (buf[0] == '\0')
        // 若未读到，返回-1
        return -1;
    else
        // 返回buf的长度
        return strlen(buf);
}



void
parsecmd(cmd *command, char *buf)
{
    char *x = buf;
    // x用于逐字分析buf
    int pos = 0;
    // pos用于记录当前argv[i]或input或output的长度
    int argc = -1;
    // argc用于记录当前argv的个数

    command->type = EXEC;
    command->input[0] = 0;
    command->output[0] = 0;

    while(*x)
    {
        if (*x != ' ' && *x != '<' && *x != '>' && *x != '|' && *x != '\n')
        {
            if (pos == 0)
            {
                argc++;
                command->argc++;
                command->argv[argc] = x;
            }
            pos++;
            x++;
        }
        else if (*x == '<')
        {
            // 输入重定向
            command->type |= REDIR;
            pos = 0;
            *x = 0;
            x++;
            while (*x && *x == ' ')
            {
                // 忽略空格
                *x = 0;
                x++;
            }
            while (*x && *x != ' ' && *x != '<' && *x != '>' && *x != '|' && *x != '\n')
            {
                command->input[pos] = *x;
                pos++;
                x++;
            }
            command->input[pos] = 0;  // input结束
        }
        else if (*x == '>')
        {
            // 输出重定向
            command->type |= REDIR;
            pos = 0;
            *x = 0;
            x++;
            while (*x && *x == ' ')
            {
                // 忽略空格
                *x = 0;
                x++;
            }
            while (*x && *x != ' ' && *x != '<' && *x != '>' && *x != '|' && *x != '\n')
            {
                command->output[pos] = *x;
                pos++;
                x++;
            }
            command->output[pos] = 0;  // output结束
        }
        else if(*x == '|')
        {
            // 管道命令
            command->type |= PIPE;
            pos = 0;
            *x = 0;
            x++;
            while (*x && *x == ' ' )
            {
                // 忽略空格
                x++;
            }
            strcpy(command->right, x); 
            break;
        }
        else
        {
            pos = 0;
            while (*x && (*x == ' ' || *x == '\n'))
            {
                *x = 0;
                x++;
            }      
        }
    }
    argc++;
    command->argc++;
    command->argv[argc] = 0;  
}



void
runcmd(cmd command)
{
    // runcmd函数调用exec系统调用加载适当的函数如：
    // echo、cat、grep等
    int p[2];
    cmd rcommand;

    // 位运算以便处理同时有REDIR和PIPE的情况
    if (command.type & REDIR)
    {
        if (strlen(command.input))
        {
            close(0);
            if (open(command.input, O_RDONLY) < 0)
            {
                fprintf(2, "open %s failed\n", command.input);
                exit(-1);
            }
        }
        if (strlen(command.output))
        {
            close(1);
            if (open(command.output, O_CREATE|O_WRONLY) < 0)
            {
                fprintf(2, "open %s failed\n", command.output);
                exit(-1);
            }
        }
    }
    
    if(command.type & PIPE)
    {
        pipe(p);
        parsecmd(&rcommand, command.right);   
        if (fork() == 0)
        {
            close(1);
            dup(p[1]);
            close(p[0]);
            close(p[1]);
            exec(command.argv[0], command.argv);
        }
        if (fork() == 0)
        {
            close(0);
            dup(p[0]);
            close(p[0]);
            close(p[1]);
            runcmd(rcommand);
        }
        close(p[0]);
        close(p[1]);
        wait(0);
        wait(0);
        exit(0);
    }

    // EXEC和REDIR执行
    exec(command.argv[0], command.argv);    
    exit(0);
}

int
main(int argc, char **argv)
{
    char buf[512];
    cmd command;
    // 以下代码部分来自sh.c以及指导书
    while (getcmd(buf, sizeof(buf)) >= 0)
    {
        if (buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' ')
        {
            // Chdir must be called by the parent, not the child.
            buf[strlen(buf) - 1] = 0;
            if (chdir(buf+3) < 0)
            {
                fprintf(2, "cannot cd %s\n", buf+3);
            }
            continue;
        }

        if (fork() == 0)
        {
            parsecmd(&command, buf);
            runcmd(command);
        }
        wait(0);
    }
    exit(0);
}
