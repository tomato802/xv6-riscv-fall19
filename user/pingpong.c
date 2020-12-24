#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    
    int parent_fd[2], child_fd[2];
    char buf[64];
    
    if (argc > 1)
    {
        fprintf(2, "usage: pingpong\n");
        exit();
    }

    pipe(parent_fd);
    pipe(child_fd);
    // 正常创建后，p[1]为管道写入端，p[0]为管道读出端
    
    if (fork() == 0)
    {
        /* Child */
        // 关闭不用的fd
        close(parent_fd[1]);
        close(child_fd[0]);
        // 读取父进程写入管道parent_fd的“ping”
        read(parent_fd[0], buf, 4);
        printf("%d: received %s\n", getpid(), buf);
        // 子进程向管道child_fd中写入“pong”
        write(child_fd[1], "pong", strlen("pong"));
    }
    else
    {
        /* Parent */
        // 关闭不用的fd
        close(parent_fd[0]);
        close(child_fd[1]);
        // 父进程向管道parent_fd中写入“ping”
        write(parent_fd[1], "ping", strlen("ping"));
        // 读取子进程写入管道parent_fd的“pong”
        read(child_fd[0], buf, 4);
        printf("%d: received %s\n", getpid(), buf);
    }

    exit();
}