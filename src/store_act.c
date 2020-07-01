#include <limits.h>
#include <stdlib.h>
#include "msg.h"
#include "passthrough.h"

int main(int argc, char *argv[])
{
    char path[PATH_MAX];
    void *cli = client_create();
    realpath(argv[0], path);
    get_file_dir(path, path);
    strcat(path, "/../conf/yadist.conf");
    config_load(path);
    set_store_root(argc, argv);
    passthrough_main(argc, argv, store_req, cli);
    client_destroy(cli);
    return 0;
}
