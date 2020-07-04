#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "msg.h"

int main(int argc, char *argv[])
{
    char path[PATH_MAX];
    setlinebuf(stdout);
    realpath(argv[0], path);
    get_file_dir(path, path);
    strcat(path, "/../conf/yadist.conf");
    config_load(path);
    char *end = config_get_server_endpoint();
    char *root = config_get_server_path();
    printf("server bind %s path %s\n", end, root);
    set_redirect_root(root);
    store_server(end);
    return 0;
}
