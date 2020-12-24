#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    if (argc != 2){
        // 检查用户是否传入正确的参数个数
        fprintf(2, "usage: sleep n\n");
        exit();
    }
    // 命令行参数为字符串，此处将其转换为整数
    sleep(atoi(argv[1]));
    exit();
}