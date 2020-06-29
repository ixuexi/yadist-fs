
#ifndef __MSG_H_INCLUDED__
#define __MSG_H_INCLUDED__

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <utime.h>

#define PATH_ABS 1
#define PATH_RELA 2

#define ST_PATH_MAX 512
#define MAX_BUF_LEN (64 * 1024)

#define path_cut(p, l) ((p) + (l))

struct req_ctx {
    void *sock;
    void *msg;
};

struct fs_info {
    mode_t st_mode;
    uid_t st_uid;
    gid_t st_gid;
    struct timespec atime;
    struct timespec mtime;
};

void set_store_root(int argc, char *argv[]);
void set_redirect_root(int argc, char *argv[]);
int store_req(void *sock, char *path);
void *client_create(void);
void client_destroy(void *sock);
void store_server(void);

int get_subdir(int argc, char *argv[], char *buf, int len);
void get_file_dir(char *dir, char *path);
void resolve_path(char path[], char *root, int rlen, char *rela, int len, int type);

#endif
