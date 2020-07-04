#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "msg.h"
#include "cmd.h"
#include "passthrough.h"

#define FUSE_OPTION "-f -o kernel_cache -o allow_other -o fsname=%s -o " \
    "modules=subdir -o subdir=%s %s"

#define OVERLAY_OPTION "-t overlay overlay -olowerdir=%s,upperdir=%s,"\
    "workdir=%s %s"

int fuse_option(char *argv[], int size)
{
    char opts[ST_PATH_MAX];
    char *subdir = config_get_fuse_subdir();
    char *mount = config_get_fuse_mount();
    (void)sprintf(opts, FUSE_OPTION, subdir, subdir, mount);
    int argc = option_vec_fill(argv, size, opts);
    option_vec_print(argv, argc);
    /* NULL terminating argv, remove last for fuse */
    argc--;
    return argc;
}

int overlay_option(char *argv[], int size)
{
    char opts[ST_PATH_MAX];
    char *lower = config_get_ov_lower();
    char *upper = config_get_ov_upper();
    char *work  = config_get_ov_work();
    char *mount = config_get_ov_mount();
    (void)sprintf(opts, OVERLAY_OPTION, lower, upper, work, mount);
    int argc = option_vec_fill(argv, size, opts);
    option_vec_print(argv, argc);
    return argc;
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
    setlinebuf(stdout);
    realpath(argv[0], path);
    get_file_dir(path, path);
    strcat(path, "/../conf/yadist.conf");
    config_load(path);
    char *end = config_get_client_endpoint();
    char *mode = config_get_client_mode();
    char *root = config_get_fuse_subdir();
    printf("client connect %s mode %s subdir %s\n", end, mode, root);
    void *cli = client_create(end);
    set_request_mode(mode);
    set_store_root(root);
    verlay_mount();
    fargv[0] = argv[0];
    fargc = fuse_option(fargv, 20);
    //fargc = overlay_option(fargv, 20);
    passthrough_main(fargc, fargv, store_req, cli);
    client_destroy(cli);
    return 0;
}
