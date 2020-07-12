
#include <limits.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include "zmq.h"
#include "czmq.h"

zsock_t *g_dish;
zsock_t *g_publ;
zhash_t *g_serh;
zhash_t *g_agth;

pthread_mutex_t g_hmutex = PTHREAD_MUTEX_INITIALIZER;

#define SW_DISH_PORT "21000"
#define SW_PUBL_PORT "22000"

#define SW_GRP_SER "server"
#define SW_GRP_AGT "agent"

#define is_ser_grp(g) (!strcmp(g, SW_GRP_SER))
#define is_agt_grp(g) (!strcmp(g, SW_GRP_AGT))

struct meminfo {
    char ip[128];
    char stat[128];
    long time;
};

void mem_init(void)
{
    g_serh = zhash_new();
    zhash_autofree(g_serh);
    g_agth = zhash_new();
    zhash_autofree(g_agth);
}

zhash_t *get_grp_hash(char *grp)
{
    if (!strcmp(grp, SW_GRP_SER))
        return g_serh;
    else
        return g_agth;
}

void mem_update(char *group, char *ip, char *stat)
{
    zhash_t *hash;
    char mbinfo[512];

    int len = sprintf(mbinfo, "%s,%s,", ip, stat);

    hash = get_grp_hash(group);
    (void)pthread_mutex_lock(&g_hmutex);
    char *val = (char *)zhash_lookup(hash, ip);
    if (val) {
        if (strncmp(val, mbinfo, len)) {
            sprintf(mbinfo + len, "%zu", time(NULL));
            zhash_update(hash, ip, mbinfo);
        }
    }
    else {
        sprintf(mbinfo + len, "%zu", time(NULL));
        zhash_insert(hash, ip, mbinfo);
    }
    (void)pthread_mutex_unlock(&g_hmutex);
}

zframe_t *mem_pack(zhash_t *h)
{
    (void)pthread_mutex_lock(&g_hmutex);
    zframe_t *f = zhash_pack(h);
    (void)pthread_mutex_unlock(&g_hmutex);
    return f;
}

void publ_create(char *port)
{
    char end[128];
    sprintf(end, "tcp://0.0.0.0:%s", port);
    zsock_t *s = zsock_new_pub(end);
    g_publ = s;
}

void publ_send(char *grp)
{
    zhash_t *h = get_grp_hash(grp);
    zframe_t *f = mem_pack(h);
    zsock_send(g_publ, "ssf", "mbinfo", grp, f);
    if (f)
        zframe_destroy(&f);
    printf("publ send to topic %s\n", grp);
}

void dish_recv(zframe_t *f, char *group)
{
    zmsg_t *msg = zmsg_decode(f);
    zframe_t *frame = zmsg_first(msg);
    char *ip = zframe_strdup(frame);
    frame = zmsg_next(msg);
    char *stat = zframe_strdup(frame);
    mem_update(group, ip, stat);
    printf("recv from %s: %s %s\n", group, ip, stat);
    zmsg_destroy(&msg);
    free(ip);
    free(stat);
}

void dish_create(char *port)
{
    char end[128];
    sprintf(end, "tcp://0.0.0.0:%s", port);
    zsock_t *s = zsock_new_dish(end);
    zsock_join(s, SW_GRP_SER);
    zsock_join(s, SW_GRP_AGT);
    g_dish = s;
}

void dish_start(void)
{
    zsock_t *s = g_dish;
    while (1) {
        zframe_t *frame = zframe_recv(s);
        if (!frame) {
            printf("dish recv null msg!\n");
            usleep(100 * 1000);
            continue;
        }
        if (is_ser_grp(zframe_group(frame)))
            dish_recv(frame, SW_GRP_SER);
        else if (is_agt_grp(zframe_group(frame)))
            dish_recv(frame, SW_GRP_AGT);
        else
            printf("dish recv unknow group %s\n", zframe_group(frame));
        zframe_destroy(&frame);
    }
}

void run_thread(void (*pfun)(void))
{
    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    int ret = pthread_create(&thread, &attr, (void *)pfun, NULL);
    printf("create thread %ld ret %d\n", thread, ret);
}


int main(void)
{
    mem_init();
    dish_create(SW_DISH_PORT);
    publ_create(SW_PUBL_PORT);
    run_thread(dish_start);
    while (1) {
        sleep(10);
        publ_send("server");
        publ_send("agent");
    } 
    return 0;
}

