#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char*
fmtname(char *path)
{
    static char buf[DIRSIZ+1];
    char *p;

    // Find first character after last slash.
    for(p=path+strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    // Return blank-padded name.
    if(strlen(p) >= DIRSIZ)
        return p;
    memmove(buf, p, strlen(p));
    memset(buf+strlen(p), '\0', 1);
    return buf;
}


void
find(char *path, char *filename)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(path, 0)) < 0)
    {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch(st.type)
    {
        case T_FILE:
            // 若为文件，则比较是否为所需寻找的文件
            if (strcmp(fmtname(path), filename) == 0)
            {
                printf("%s\n", path);
                return;
            }
            break;
            
        case T_DIR:
            // 若为目录，则继续递归调用find
            if (strlen(path) + 1 + DIRSIZ + 1 > sizeof (buf))
            {
                fprintf(2, "find: path too long\n");
                break;
            }
            strcpy(buf, path);
            p = buf+strlen(buf);
            *p++ = '/';
            while(read(fd, &de, sizeof(de)) == sizeof(de))
            {
                // 依次读取fd下各个文件名或目录名
                if (de.inum == 0 || de.name == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                    continue;
                // 拷贝文件名或目录名到buf/后
                memmove(p, de.name, DIRSIZ);
                p[strlen(de.name)] = '\0';
                find(buf, filename);
            }
            break;
    }
    close(fd);
}


int
main(int argc, char *argv[])
{    
    if (argc != 3)
    {
        fprintf(2, "usage: find path filename\n");
        exit();
    }
    find(argv[1], argv[2]);
    exit();
}