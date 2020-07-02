#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "msg.h"
#include "passthrough.h"

int fuse_option(char *argv[])
{
    char sub[ST_PATH_MAX];
    char fsn[ST_PATH_MAX];
    int argc = 1;
    char *subdir = config_get_fuse_subdir();
    char *mount = config_get_fuse_mount();
    fsn[0] = 0;
    strcat(fsn, "fsname=");
    strcat(fsn, subdir);
    sub[0] = 0;
    strcat(sub, "subdir=");
    strcat(sub, subdir);
    argv[argc++] = "-f";
    argv[argc++] = "-o";
    argv[argc++] = "kernel_cache";
    argv[argc++] = "-o";
    argv[argc++] = "allow_other";
    argv[argc++] = "-o";
    argv[argc++] = fsn;
    argv[argc++] = "-o";
    argv[argc++] = "modules=subdir";
    argv[argc++] = "-o";
    argv[argc++] = sub;
    argv[argc++] = mount;
    return argc;
}

void do_verlay_mount(void)
{

}

void verlay_mount(void)
{
//    zloop_t *loop = zloop_new();
//    zloop_timer(loop, 3, 0, do_verlay_mount, NULL);
}

int main(int argc, char *argv[])
{
    char path[PATH_MAX];
    char *fargv[20];
    int fargc;
    realpath(argv[0], path);
    get_file_dir(path, path);
    strcat(path, "/../conf/yadist.conf");
    config_load(path);
    char *end = config_get_client_endpoint();
    char *mode = config_get_client_mode();
    printf("client connect %s mode %s\n", end, mode);
    void *cli = client_create(end);
    verlay_mount();
    fargv[0] = argv[0];
    fargc = fuse_option(fargv);
    set_store_root(fargc, fargv);
    passthrough_main(fargc, fargv, store_req, cli);
    client_destroy(cli);
    return 0;
}
