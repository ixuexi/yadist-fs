#include "msg.h"

int main(int argc, char *argv[])
{
    char path[PATH_MAX];
    realpath(argv[0], path);
    get_file_dir(path, path);
    strcat(path, "/../conf/yadist.conf");
    config_load(path);
    set_redirect_root(argc, argv);
    store_server();
    return 0;
}
