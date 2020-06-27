#include "czmq.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

void start_server(void)
{
    zsock_t *s = zsock_new_server ("inproc://zmsg.test");
    while (1) {
        zmsg_t *msg;
        zframe_t *fr;
        char *data;
        zsock_recv(s, "s", &data);
        if (!data) {
            printf("recv null msg!\n");
            continue;
        }
        printf("recv %s\n", data);
        /*
        printf("recv msg from %u\n", zmsg_routing_id(msg));
        fr = zmsg_pop(msg);
        data = zframe_data(fr);
        printf("@1st: %s\n", (char *)data);
        zframe_destroy(&fr);
        fr = zmsg_pop(msg);
        data = zframe_data(fr);
        printf("@2st: %d\n", *(int *)data);
        zframe_destroy(&fr);
        zmsg_destroy(&msg);
        */
    }
    zsock_destroy(&s);
}

void start_client(void)
{
    zsock_t *s = zsock_new_client ("inproc://zmsg.test");
    while (1) {
        /*
        int value = 0x12345678;
        zmsg_t *msg = zmsg_new();
        zmsg_addstr(msg, "hello");
        zmsg_addmem(msg, &value, sizeof(value));
        {
            char *data;
            zframe_t *fr;
            fr = zmsg_first(msg);
            data = zframe_data(fr);
            printf(">1st: %s\n", (char *)data);
            fr = zmsg_next(msg);
            data = zframe_data(fr);
            printf(">2st: %d\n", *(int *)data);
        }
        printf(">send size %d\n", zmsg_size(msg));
        zmsg_send(&msg, s);
        */
        zstr_send(s, "hello");
        (void)sleep(1);
    }
    zsock_destroy(&s);
}

int main(int argc, char *argv[])
{
    pthread_attr_t attr;
    pthread_t t;
    pthread_attr_init(&attr);
    pthread_create(&t, &attr, start_server, NULL);
    pthread_create(&t, &attr, start_client, NULL);
    sleep(100);
    return 0;
}
