/* xsh_hello.c - xsh_hello */

#include <xinu.h>
#include <stdio.h>

shellcmd xsh_malloc_test(int nargs, char *args[])
{
    void *b1 = malloc(40);
    printf(heap_snapshot());
    void *b2 = malloc(2);
    printf(heap_snapshot());
    void *b3 = malloc(8);
    printf(heap_snapshot());

    free(b1);
    printf(heap_snapshot());
    free(b2);
    printf(heap_snapshot());
    free(b3);
    printf(heap_snapshot());

    return SHELL_OK;
}