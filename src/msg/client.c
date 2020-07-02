#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <time.h>
#include "msg.h"
#include "hash.h"
#include "zmq.h"
#include "czmq.h"
#include "client.h"

void *g_shash;
char g_sr_path[ST_PATH_MAX];
int g_sr_plen;
char *g_mode;
/*
 * temp lock, fuse muti-work thread conflict
 */
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

void set_store_root(int argc, char *argv[])
{
    g_sr_plen = get_subdir(argc, argv, g_sr_path, ST_PATH_MAX);
    printf("store root dir=%s len=%d\n", g_sr_path, g_sr_plen);
}

void set_request_mode(char *mode)
{
    g_mode = mode;
}

char *get_request_mode(void)
{
    return g_mode;
}

int tx_file(char *cmd, zmsg_t *msg)
{
    zframe_t *frame = zmsg_next(msg);;
    struct fs_info *info;
    struct timeval times[2];
    char path[ST_PATH_MAX];
    size_t mlen;
    int fd;
    resolve_path(path, g_sr_path, g_sr_plen, zframe_data(frame), zframe_size(frame), PATH_ABS);
    if (!strncmp(cmd + 8, "CT", 2)) {
        fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0777);
        printf("recv file %s create ret %d\n", path, fd);
        frame = zmsg_next(msg);
        if (frame) {
            info = (struct fs_info *)zframe_data(frame);
            fchown(fd, info->st_uid, info->st_gid);
            fchmod(fd, info->st_mode);
            memcpy(&times, &(info->atime), sizeof(times));
            times[0].tv_usec /= 1000;
            times[1].tv_usec /= 1000;
            utimes(path, times);
        }
    }
    else if (!strncmp(cmd + 8, "AP", 2)) {
        fd = open(path, O_WRONLY | O_APPEND);
        printf("recv file %s append ret %d\n", path, fd);
    }
    else {
        printf("error: file cmd %s\n", cmd);
        return -1;
    }
    frame = zmsg_next(msg);
    if (fd > 0 && frame) {
        mlen = zframe_size(frame);
        write(fd, zframe_data(frame), mlen);
        printf("write file %s len %zd\n", path, mlen);
        close(fd);
    }
    else {
        printf("write file error fd %d msg %p\n", fd, frame);
        return -1;
    }
    return 0;
}

int tx_dir_node(char *cmd, zmsg_t *msg)
{
    zframe_t *frame;
    struct fs_info *info;
    struct timeval times[2];
    char path[ST_PATH_MAX];
    int ret;
    frame = zmsg_next(msg);
    resolve_path(path, g_sr_path, g_sr_plen, zframe_data(frame), zframe_size(frame), PATH_ABS);
    ret = mkdir(path, 0777);
    printf("create dir %s ret %d\n", path, ret);
    frame = zmsg_next(msg);
    if (frame) {
        info = (struct fs_info *)zframe_data(frame);
        chown(path, info->st_uid, info->st_gid);
        chmod(path, info->st_mode);
        memcpy(&times, &(info->atime), sizeof(times));
        times[0].tv_usec /= 1000;
        times[1].tv_usec /= 1000;
        utimes(path, times);
    }
    return ret;
}

int tx_link(char *cmd, zmsg_t *msg)
{
    zframe_t *frame;
    char path[ST_PATH_MAX];
    char pdir[ST_PATH_MAX];
    char oldpath[ST_PATH_MAX];
    char *linfo;
    int ret;
    frame = zmsg_next(msg);
    resolve_path(path, g_sr_path, g_sr_plen, zframe_data(frame), zframe_size(frame), PATH_ABS);
    frame = zmsg_next(msg);
    linfo = zframe_data(frame);
    resolve_path(oldpath, g_sr_path, g_sr_plen, zframe_data(frame), zframe_size(frame), PATH_RELA);
    get_file_dir(pdir, path);
    chdir(pdir);
    ret = symlink(oldpath, path);
    printf("symlink %s -> %s ret %d\n", path, oldpath, ret);
    return ret;
}

/*
 CMD: TX_FILE_CT: trans create
      TX_FILE_AP: trans append
      TX_DIR:  tran dir
      TX_LINK: tran link
      TX_END:  tran req end
*/
int proc_rep_msg(zsock_t *s, zmsg_t *msg)
{
    char cmd[ST_PATH_MAX];
    zframe_t *frame = zmsg_first(msg);
    size_t mlen = zframe_size(frame);
    int ret = PROC_MSG_OK;
    if (mlen >= ST_PATH_MAX) {
        mlen = ST_PATH_MAX - 1;
    }
    memcpy(cmd, zframe_data(frame), mlen);
    cmd[mlen] = 0;
    printf("recv cmd %s\n", cmd);
    if (!strncmp(cmd, "TX_FILE", 7)) {
        if (tx_file(cmd, msg))
            ret = PROC_MSG_ERR;
    }
    else if (!strncmp(cmd, "TX_DIR", 6)) {
        if (tx_dir_node(cmd, msg))
            ret = PROC_MSG_ERR;
    }
    else if (!strncmp(cmd, "TX_LINK", 7)) {
        if (tx_link(cmd, msg))
            ret = PROC_MSG_ERR;
    }
    else if (!strncmp(cmd, "TX_END", 6)) {
        ret = PROC_MSG_END;
    }
    else {
        ret = PROC_MSG_ERR;
    }
    zmsg_destroy(&msg);
    return ret;
}

int store_req(void *sock, char *path)
{
    zsock_t *s = (zsock_t *)sock;
    zmsg_t *msg;
    char *real = path + g_sr_plen;
    int count = 0;
    if (shash_lookup(g_shash, real)) {
        //printf("file %s has been trans\n", path);
        return -1;
    }
    (void)pthread_mutex_lock(&g_mutex);
    shash_insert(g_shash, real);
    msg = zmsg_new();
    zmsg_addstr(msg, real);
    zmsg_addstr(msg, get_request_mode());
    int ret = zmsg_send(&msg, s);
    printf("send req path %s ret %d\n", real, ret);
    while (1) {
        zmsg_t *msg = zmsg_recv(s);
        if (!msg) {
            printf("recv null msg!\n");
            continue;
        }
        int ret = proc_rep_msg(s, msg);
        if (ret == PROC_MSG_OK)
            count++;
        else if (ret == PROC_MSG_END)
            break;
    }
    if (count > 0) {
        printf("total tx %d files\n", count);
        int l = pthread_mutex_unlock(&g_mutex);
        return 0;
    }
    else {
        printf("req path %s has no result!\n", real);
        int l = pthread_mutex_unlock(&g_mutex);
        return -1;
    }
}

void *client_create(char *endpoint)
{
    const char *url = endpoint;
    zsock_t *s = zsock_new_pair(url);
    g_shash = shash_create();
    printf("connect to url %s ret %p\n", url, s);
    return s;
}

void client_destroy(void *sock)
{
    zsock_t *s = (zsock_t *)sock;
    zsock_destroy(&s);
}

#ifdef TEST
void store_client(char *path)
{
    void *s = client_create();
    int ret store_req(s, path);
    printf("request ret %d\n", ret);
    client_destroy(s);
}

int main(int argc, char *argv[])
{
    store_client(argv[1]);
    return 0;
}
#endif
