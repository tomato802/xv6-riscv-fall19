#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int
main(int argc, char *argv[])
{
    int i, j,  n, pos;
    char buf[128];
    char buf2[128];
    char *temp = buf2;
    char *eargv[MAXARG];
    if (argc < 2)
    {
        fprintf(2, "usage: somecommand | xargs command\n");
        exit();
    }

    for (i = 1; i < argc; i++)
    {
        eargv[i-1] = argv[i];
    }
    
    while ((n = read(0, buf, sizeof(buf))) > 0) 
    {
        pos = argc - 1; // 记录eagrv的位置
        j = 0;
        eargv[pos] = temp;
        pos++;
        for (i = 0; i < n; i++)
        {
            if (buf[i] == ' ' || buf[i] == '\n')
            {
                buf2[j] = '\0';
                j++;
                temp = &buf2[j];
                eargv[pos] = temp;
                pos++;
            }
            else
            {
                buf2[j] = buf[i];
                j++;
            }
        }
        
        eargv[pos] = 0;

       
        if (fork() == 0)
        {
            exec(argv[1], eargv);
        }
        else
        {
            wait();
        }
    }

    if (n < 0)
    {
        printf("xargs: read error\n");
    }
      
    exit();
}