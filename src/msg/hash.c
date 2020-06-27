#include "czmq.h"
#define SHASH_NIL "nil"

void *shash_create(void)
{
    zhash_t *hash = zhash_new ();
    return hash;    
}

int shash_insert(void *hash, char *str)
{
    zhash_t *h = (zhash_t *)hash;
    return zhash_insert(h, str, SHASH_NIL);
}

char *shash_lookup(void *hash, char *str)
{
    zhash_t *h = (zhash_t *)hash;
    return zhash_lookup(h, str);
}
