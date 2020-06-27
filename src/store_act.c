#include "msg.h"
#include "passthrough.h"

int main(int argc, char *argv[])
{
    void *cli = client_create();
    set_store_root(argc, argv);
    passthrough_main(argc, argv, store_req, cli);
    client_destroy(cli);
    return 0;
}
