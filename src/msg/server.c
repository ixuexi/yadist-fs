#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>
#include "msg.h"
#include "zmq.h"
#include "czmq.h"

char *g_fbuf;

char *file_buf(void)
{
    if (!g_fbuf)
        g_fbuf = malloc(MAX_BUF_LEN);
    return g_fbuf;
}

void get_file_info(int fd, struct fs_info *info)
{
    struct stat buf;
    int ret = fstat(fd, &buf);
    info->st_mode = buf.st_mode;
    info->st_uid = buf.st_uid;
    info->st_gid = buf.st_gid;
    info->atime = buf.st_atim;
    info->mtime = buf.st_mtim;
}

void file_tran(char *name, struct req_ctx *ctx)
{
    char *buf = file_buf();
    struct fs_info info;
    int ret, rlen, fd;
    int tb = 0;
    fd = open(name, O_RDONLY);
    if (fd < 0)
        return;
    do {
        rlen = read(fd, buf, MAX_BUF_LEN);
        if (rlen <= 0) {
            printf("error read %s ret %d\n", name, rlen);
            break;
        }
        zmsg_t *msg = zmsg_new();
        if (tb++ == 0) {
            get_file_info(fd, &info);
            zmsg_addstr(msg, "TX_FILE_CT");
            zmsg_addstr(msg, name);
            zmsg_addmem(msg, &info, sizeof(info));
        }
        else {
            zmsg_addstr(msg, "TX_FILE_AP");
            zmsg_addstr(msg, name);
        }
        zmsg_addmem(msg, buf, rlen);
        ret = zmsg_send(&(msg), (zsock_t *)ctx->sock);
        printf("tran file %s len %d ret %d\n", name, rlen, ret);
    }
    while(rlen == MAX_BUF_LEN);
    close(fd);
}

void get_dir_info(char *path, struct fs_info *info)
{
    struct stat buf;
    int ret = stat(path, &buf);
    info->st_mode = buf.st_mode;
    info->st_uid = buf.st_uid;
    info->st_gid = buf.st_gid;
    info->atime = buf.st_atim;
    info->mtime = buf.st_mtim;
}

void dir_node_tran(char *path, struct req_ctx *ctx)
{
    int ret;
    struct fs_info info;
    zmsg_t *msg = zmsg_new();
    zmsg_addstr(msg, "TX_DIR");
    zmsg_addstr(msg, path);
    get_dir_info(path, &info);
    zmsg_addmem(msg, &info, sizeof(info));
    ret = zmsg_send(&(msg), (zsock_t *)ctx->sock);
    printf("send mkdir msg ret %d\n", ret);
}

void link_tran(char *path, struct req_ctx *ctx)
{
    int ret;
    char buf[ST_PATH_MAX];
    int len = readlink(path, buf, ST_PATH_MAX - 1);
    if (len > 0 && len < (ST_PATH_MAX - 1)) {
        zmsg_t *msg = zmsg_new();
        zmsg_addstr(msg, "TX_LINK");
        zmsg_addstr(msg, path);
        zmsg_addmem(msg, buf, len);
        ret = zmsg_send(&(msg), (zsock_t *)ctx->sock);
        printf("send link msg ret %d\n", ret);
    }
    printf("readlink %s ret %d\n", path, len);
}

void end_tran(struct req_ctx *ctx)
{
    int ret;
    zmsg_t *msg = zmsg_new();
    zmsg_addstr(msg, "TX_END");
    ret = zmsg_send(&(msg), (zsock_t *)ctx->sock);
    printf("send end msg ret %d\n", ret);
}

void process_req_path(char *path, struct req_ctx *ctx)
{
    DIR *p = opendir(path);
    char fname[512];
    zmsg_t *msg;
    int ret;
    printf("open path %s dir %p\n", path, p);
    if (!p) {
        file_tran(path, ctx);
        goto end_msg;
    }
    dir_node_tran(path, ctx);
    for (;;) {
        struct dirent *d = readdir(p);
        if (d == NULL)
            break;
        (void)sprintf(fname, "%s/%s", path, d->d_name);
        switch(d->d_type) {
            case DT_REG:
                file_tran(fname, ctx); break;
            case DT_DIR:
                dir_node_tran(fname, ctx); break;
            case DT_LNK:
                link_tran(fname, ctx); break;
            default:
                printf("ignore file %s type %d\n", fname, d->d_type);
        }
    }
    if (p) {
        closedir(p);
    }

end_msg:
    end_tran(ctx);
}

int proc_req_msg(zsock_t *s, zmsg_t *msg)
{
    char path[ST_PATH_MAX];
    struct req_ctx ctx = {s, NULL};
    zframe_t *frame = zmsg_first(msg);
    size_t mlen = zframe_size(frame);
    struct store_req *req;
    if (mlen >= ST_PATH_MAX) {
        printf("error msg len %zd\n", mlen);
        return -1;
    }
    memcpy(path, zframe_data(frame), mlen);
    path[mlen] = 0;
    zmsg_destroy(&msg);
    process_req_path(path, &ctx);
    return 0;
}

void store_server(void)
{
    const char *url = "@tcp://127.0.0.1:10000";
    zsock_t *s = zsock_new_pair(url);
    printf("start store server at %s\n", url);
    while (1) {
        zmsg_t *msg = zmsg_recv(s);
        if (!msg) {
            printf("recv null msg!\n");
            continue;
        }
        printf("recv msg %p\n", msg);
        (void)proc_req_msg(s, msg);
    }
    zsock_destroy(&s);
}

#ifdef TEST
int main(void)
{
    store_server();
    return 0;
}
#endif
