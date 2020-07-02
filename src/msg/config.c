#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "czmq.h"
#include "msg.h"

zconfig_t *g_config;

void config_load(char *path)
{
    g_config = zconfig_load(path);
}

char *config_get_server_endpoint(void)
{
    return zconfig_get(g_config, "/server/bind", "0.0.0.0:10000");
}

char *config_get_client_endpoint(void)
{
    return zconfig_get(g_config, "/client/connect", "0.0.0.0:9999");
}

char *config_get_client_mode(void)
{
    return zconfig_get(g_config, "/client/mode", "ONDEMAND");
}

char *config_get_fuse_subdir(void)
{
    return zconfig_get(g_config, "/fuse/subdir", "");
}

char *config_get_fuse_mount(void)
{
    return zconfig_get(g_config, "/fuse/mount", "");
}

char *config_get_ov_lower(void)
{
    return zconfig_get(g_config, "/overlay/lower", "");
}

char *config_get_ov_upper(void)
{
    return zconfig_get(g_config, "/overlay/upper", "");
}

char *config_get_ov_work(void)
{
    return zconfig_get(g_config, "/overlay/work", "");
}
#ifdef TEST
int main(int argc, char *argv[])
{
    char rpath[PATH_MAX];
    realpath(argv[0], rpath);
    get_file_dir(rpath, rpath);
    strcat(rpath, "/../conf/yadist.conf");
    zconfig_t *conf = zconfig_load (rpath);
    config_load(rpath);
    printf(config_get_server_endpoint());
    printf(config_get_client_endpoint());
    printf(config_get_fuse_subdir());
    printf(config_get_fuse_mount());
    printf(config_get_ov_lower());
    printf(config_get_ov_upper());
    printf(config_get_ov_work());
    return 0;
}
#endif
