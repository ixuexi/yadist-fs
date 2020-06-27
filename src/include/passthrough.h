
#ifndef __PASSTHROUGH_H_INCLUDED__
#define __PASSTHROUGH_H_INCLUDED__

int passthrough_main(int argc, char *argv[], int (*pfn_req)(void *, char *), void *sock);

#endif
