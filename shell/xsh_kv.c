/* xsh_kv.c - xsh_kv */

#include <xinu.h>
#include <stdio.h>

shellcmd xsh_kv(int nargs, char *args[])
{
    // int retval; 
    // int set_errors = 0; 
    // char* valtmp=NULL; 
    //  int get_errors=0; 
    replacement_policy_t policy = ARC;
    kv_init(policy); 
    
    kv_set("name", "Saurabh");
    kv_set("algorithm", "ARC");
    kv_set("os", "XINU");
    
    printf("name = %s\n", kv_get("name"));
    printf("algorithm = %s\n", kv_get("algorithm"));
    char* names[] = {"total_hits", "total_accesses", "cache_size", "total_set_success", "num_keys", "total_evictions" };
    int i;
    for (i=0; i<6; i++) 
    {
        printf("%s=%d\n", names[i], get_cache_info(names[i]));
    }
    char **popular = most_popular_keys(2);
    for(i=0; i<2; i++)
    {
        printf("%s\n", popular[i]);
    }
    xfree(popular);
    kv_reset();
    return SHELL_OK;
}
