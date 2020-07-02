#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "msg.h"

int main(int argc, char *argv[])
{
    char path[PATH_MAX];
    realpath(argv[0], path);
    get_file_dir(path, path);
    strcat(path, "/../conf/yadist.conf");
    config_load(path);
    char *end = config_get_server_endpoint();
    printf("server bind %s\n", end);
    set_redirect_root(argc, argv);
    store_server(end);
    return 0;
}
