
#ifndef __HASH_H_INCLUDED__
#define __HASH_H_INCLUDED__

void *shash_create(void);
int shash_insert(void *hash, char *str);
char *shash_lookup(void *hash, char *str);


#endif
