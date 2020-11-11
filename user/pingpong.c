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
    
    if (fork() == 0)
    {
        /* Child */
        close(parent_fd[1]);
        close(child_fd[0]);
        // 关闭不用的fd
        read(parent_fd[0], buf, 4);
        fprintf(2, "%d: received %s\n", getpid(), buf);
        write(child_fd[1], "pong", strlen("pong"));
    }
    else
    {
        /* Parent */
        close(parent_fd[0]);
        close(child_fd[1]);
        // 关闭不用的fd
        write(parent_fd[1], "ping", strlen("ping"));
        read(child_fd[0], buf, 4);
        fprintf(2, "%d: received %s\n", getpid(), buf);
    }

    exit();
}