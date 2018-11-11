/* xsh_kv.c - xsh_kv */

#include <stdio.h>
#include <xinu.h>

shellcmd xsh_kv(int nargs, char *args[])
{
    kv_init();

    kv_set("course", "Advanced Operating Systems");
    printf("%s\n", kv_get("course"));
    return SHELL_OK;
}