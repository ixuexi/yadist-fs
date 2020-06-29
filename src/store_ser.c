#include "msg.h"

int main(int argc, char *argv[])
{
    set_redirect_root(argc, argv);
    store_server();
    return 0;
}
