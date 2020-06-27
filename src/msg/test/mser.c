#include "msg.h"
#include <stdio.h>

void start_server(void)
{
    zsock_t *s = zsock_new_rep("tcp://127.0.0.1:10000");
    while (1) {
        zmsg_t *msg = zmsg_recv(s);
        zframe_t *fr;
        char *data;
        if (!msg) {
            printf("recv null msg!\n");
            continue;
        }
        fr = zmsg_first(msg);
        if (fr) {
            data = zframe_data(fr);
            printf("1st: %s\n", (char *)data);
        }
        fr = zmsg_next(msg);
        if (fr) {
            data = zframe_data(fr);
            printf("2st: %d\n", *(int *)data);
        }
        zmsg_send(&msg, s);
    }
    zsock_destroy(&s);
}

int main(int argc, char *argv[])
{
    start_server();
    return 0;
}
