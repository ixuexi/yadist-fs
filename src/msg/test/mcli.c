#include "zmq.h"
#include "czmq.h"
#include <stdio.h>
#include <unistd.h>

void start_client(char *id)
{
    zsock_t *s = zsock_new_req("tcp://127.0.0.1:10000");
    int value = 0;
    while (1) {
        int ret;
        zframe_t *fr;
        char *data;
        zmsg_t *msg = zmsg_new();
        ret = zmsg_addstr(msg, id);
        value++;
        printf("value %d\n", value);
        zmsg_addmem(msg, &value, sizeof(value));
        ret = zmsg_send(&msg, s);
        printf("send ret %d\n", ret);
        msg = zmsg_recv(s);
        printf("recv msg %p\n", msg);
        fr = zmsg_first(msg);
        if (fr) {
            data = zframe_data(fr);
            printf("===1st: %s\n", (char *)data);
        }
        fr = zmsg_next(msg);
        if (fr) {
            data = zframe_data(fr);
            printf("===2st: %d\n", *(int *)data);
        }
        (void)sleep(1);
    }
    zsock_destroy(&s);
}

int main(int argc, char *argv[])
{
    start_client(argv[1]);
    return 0;
}
