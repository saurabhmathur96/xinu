/* xsh_xmalloc_test.c - xsh_xmalloc_test */

#include <xinu.h>
#include <stdio.h>

shellcmd xsh_xmalloc_test(int nargs, char *args[])
{
    xmalloc_init();
    void *b1 = xmalloc(40);
    printf(heap_snapshot());
    void *b2 = xmalloc(2);
    printf(heap_snapshot());
    void *b3 = xmalloc(8);
    printf(heap_snapshot());

    xfree(b1);
    printf(heap_snapshot());
    xfree(b2);
    printf(heap_snapshot());
    xfree(b3);
    printf(heap_snapshot());

    return SHELL_OK;
}