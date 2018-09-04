/* xsh_hello.c - xsh_hello */

#include <xinu.h>
#include <stdio.h>

shellcmd xsh_hello(int nargs, char *args[])
{
    if (nargs < 2)
    {
        fprintf(stderr, "Too few arguments\n");
        return SHELL_ERROR;
    }

    if (nargs > 2)
    {
        fprintf(stderr, "Too many arguments\n");
        return SHELL_ERROR;
    }

    printf("Hello %s, Welcome to the world of Xinu!!\n", args[1]);

    return SHELL_OK;
}