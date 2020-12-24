#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
source(int *fd)
{
    int i;
    for (i = 2; i <= 35; i++)
    {
        write(fd[1], &i, sizeof(i));
    }
}

void
sink(int *fd)
{
    int prime, x;
    int fd_next[2];
    if (read(fd[0], &prime, sizeof(prime)))
    {
        printf("prime %d\n", prime);
        pipe(fd_next);
        if (fork() == 0)
        {
            /* Child */
            // 关闭下一个管道的读入端，仅用于写入
            close(fd_next[0]);
            while (read(fd[0], &x, sizeof(x)))
            {
                if (x % prime != 0)
                {
                    // 排除掉prime的倍数后，写入下一管道
                    write(fd_next[1], &x, sizeof(x));
                }
            }
        }
        else
        {
            /* Parent */
            close(fd_next[1]);
            sink(fd_next);
        } 
    }
}


int
main(int argc, char *argv[])
{
    int fd[2];

    if (argc > 1)
    {
        printf("usage: primes\n");
        exit();
    }

    pipe(fd);

    if (fork() == 0)
    {
        /* Child */
        close(fd[0]);
        source(fd);
    }
    else
    {
        /* Parent */
        close(fd[1]);
        sink(fd);
        // 等待子进程结束
        wait();
    }
    exit();
}
