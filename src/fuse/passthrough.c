/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  Copyright (C) 2011       Sebastian Pipping <sebastian@pipping.org>

  This program can be distributed under the terms of the GNU GPLv2.
  See the file COPYING.
*/

/** @file
 *
 * This file system mirrors the existing file system hierarchy of the
 * system, starting at the root file system. This is implemented by
 * just "passing through" all requests to the corresponding user-space
 * libc functions. Its performance is terrible.
 *
 * Compile with
 *
 *     gcc -Wall passthrough.c `pkg-config fuse3 --cflags --libs` -o passthrough
 *
 * ## Source code ##
 * \include passthrough.c
 */


#define FUSE_USE_VERSION 31

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _GNU_SOURCE

#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#ifdef __FreeBSD__
#include <sys/socket.h>
#include <sys/un.h>
#endif
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#include "passthrough_helpers.h"

#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include "hash.h"

char g_root_path[256];
char g_root_plen;
int (*g_pfn_req)(void *, char *);
void *g_sock;

void get_pass_subdir(int argc, char *argv[])
{
    int i;
    for (i = 1; i < argc; i++) 
        if (!strncmp(argv[i], "subdir=", 7)) {
            strncpy(g_root_path, argv[i] + 7, 255);
            g_root_plen = strnlen(g_root_path, 255);
            printf("root dir=%s len=%d\n", g_root_path, g_root_plen);
            break;
        }
}

int filter_out(const char *path)
{
	const char *rela_p = path + g_root_plen;
	if (!strncmp(rela_p, "/proc", 5))
		return 1;
	if (!strncmp(rela_p, "/sys", 4))
		return 1;
	if (!strncmp(rela_p, "/dev", 4))
		return 1;
	return 0;
}

int do_fs_fault(const char *where, const char *path, int is_file)
{
    int err_bak = errno;
    int ret = -1;
	if (errno != ENOENT) {
		ret = -1;
        goto proc_end;
	}
	if (filter_out(path)) {
		ret = -1;
        goto proc_end;
	}
	//printf("%s fault path: %s is file %d\n", where, path, is_file);
    if (g_pfn_req)
	    ret = g_pfn_req(g_sock, (char *)path);
proc_end:
    if (!ret)
        errno = 0;
    else
        errno = err_bak;
    return ret;
}

static int path_isdir(const char *path)
{
    struct stat statbuf;
    if(!stat(path, &statbuf))
        return S_ISDIR(statbuf.st_mode);
    return 0;
}

static void *xmp_init(struct fuse_conn_info *conn,
		      struct fuse_config *cfg)
{
	(void) conn;
	cfg->use_ino = 1;

	/* Pick up changes from lower filesystem right away. This is
	   also necessary for better hardlink support. When the kernel
	   calls the unlink() handler, it does not know the inode of
	   the to-be-removed entry and can therefore not invalidate
	   the cache of the associated inode - resulting in an
	   incorrect st_nlink value being reported for any remaining
	   hardlinks to this inode. */
	cfg->entry_timeout = 0;
	cfg->attr_timeout = 0;
	cfg->negative_timeout = 0;

	return NULL;
}

static int xmp_getattr(const char *path, struct stat *stbuf,
		       struct fuse_file_info *fi)
{
	(void) fi;
	int res;
	int tried = 0;
    if (path_isdir(path)) {
        errno = ENOENT;
        (void)do_fs_fault("getattr", path, 0);
        tried = 1;
    }

retry:
	res = lstat(path, stbuf);
	if (res == -1) {
		if (!tried && !do_fs_fault("getattr", path, 1)) {
			tried = 1;
			goto retry;
		}
		return -errno;
	}

	return 0;
}

static int xmp_access(const char *path, int mask)
{
	int res;
	int tried = 0;

retry:
	res = access(path, mask);
	if (res == -1) {
		if (!tried && !do_fs_fault("access", path, 1)) {
			tried = 1;
			goto retry;
		}
		return -errno;
	}

	return 0;
}

static int xmp_readlink(const char *path, char *buf, size_t size)
{
	int res;
	int tried = 0;

retry:
	res = readlink(path, buf, size - 1);
	if (res == -1) {
		if (!tried && !do_fs_fault("readlink", path, 1)) {
			tried = 1;
			goto retry;
		}
		return -errno;
	}

	buf[res] = '\0';
	return 0;
}


static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi,
		       enum fuse_readdir_flags flags)
{
	DIR *dp;
	struct dirent *de;
	int tried = 0;

	(void) offset;
	(void) fi;
	(void) flags;

retry:
	dp = opendir(path);
	if (dp == NULL) {
		if (!tried && !do_fs_fault("readdir", path, 0)) {
			tried = 1;
			goto retry;
		}
		return -errno;
	}

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0, 0))
			break;
	}

	closedir(dp);
	return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res;
	int tried = 0;

retry:
	res = mknod_wrapper(AT_FDCWD, path, NULL, mode, rdev);
	if (res == -1) {
		if (!tried && !do_fs_fault("mknod", path, 1)) {
			tried = 1;
			goto retry;
		}
		return -errno;
	}

	return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
	int res;
	int tried = 0;

retry:
	res = mkdir(path, mode);
	if (res == -1) {
		if (!tried && !do_fs_fault("mkdir", path, 0)) {
			tried = 1;
			goto retry;
		}
		return -errno;
	}

	return 0;
}

static int xmp_unlink(const char *path)
{
	int res;

	res = unlink(path);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rmdir(const char *path)
{
	int res;

	res = rmdir(path);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_symlink(const char *from, const char *to)
{
	int res;
	int tried = 0;

retry:
	res = symlink(from, to);
	if (res == -1) {
		if (!tried && !do_fs_fault("symlink", to, 0)) {
			tried = 1;
			goto retry;
		}
		return -errno;
	}

	return 0;
}

static int xmp_rename(const char *from, const char *to, unsigned int flags)
{
	int res;
	int tried = 0;

	if (flags)
		return -EINVAL;

retry:
	res = rename(from, to);
	if (res == -1) {
		if (!tried && !do_fs_fault("rename", from, 0)) {
			tried = 1;
			goto retry;
		}
		return -errno;
	}

	return 0;
}

static int xmp_link(const char *from, const char *to)
{
	int res;
	int tried = 0;

retry:
	res = link(from, to);
	if (res == -1) {
		if (!tried && !do_fs_fault("link", to, 0)) {
			tried = 1;
			goto retry;
		}
		return -errno;
	}

	return 0;
}

static int xmp_chmod(const char *path, mode_t mode,
		     struct fuse_file_info *fi)
{
	(void) fi;
	int res;
	int tried = 0;

retry:
	res = chmod(path, mode);
	if (res == -1) {
		if (!tried && !do_fs_fault("chmod", path, 1)) {
			tried = 1;
			goto retry;
		}
		return -errno;
	}

	return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid,
		     struct fuse_file_info *fi)
{
	(void) fi;
	int res;
	int tried = 0;

retry:
	res = lchown(path, uid, gid);
	if (res == -1) {
		if (!tried && !do_fs_fault("chown", path, 1)) {
			tried = 1;
			goto retry;
		}
		return -errno;
	}

	return 0;
}

static int xmp_truncate(const char *path, off_t size,
			struct fuse_file_info *fi)
{
	int res;

	if (fi != NULL)
		res = ftruncate(fi->fh, size);
	else
		res = truncate(path, size);
	if (res == -1)
		return -errno;

	return 0;
}

#ifdef HAVE_UTIMENSAT
static int xmp_utimens(const char *path, const struct timespec ts[2],
		       struct fuse_file_info *fi)
{
	(void) fi;
	int res;

	/* don't use utime/utimes since they follow symlinks */
	res = utimensat(0, path, ts, AT_SYMLINK_NOFOLLOW);
	if (res == -1)
		return -errno;

	return 0;
}
#endif

static int xmp_create(const char *path, mode_t mode,
		      struct fuse_file_info *fi)
{
	int res;
	int tried = 0;

retry:
	res = open(path, fi->flags, mode);
	if (res == -1) {
		if (!tried && !do_fs_fault("create", path, 1)) {
			tried = 1;
			goto retry;
		}
		return -errno;
	}

	fi->fh = res;
	return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
	int res;
	int tried = 0;

retry:
	res = open(path, fi->flags);
	if (res == -1) {
		if (!tried && !do_fs_fault("open", path, 1)) {
			tried = 1;
			goto retry;
		}
		return -errno;
	}

	fi->fh = res;
	return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	int fd;
	int res;

	if(fi == NULL)
		fd = open(path, O_RDONLY);
	else
		fd = fi->fh;
	
	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	if(fi == NULL)
		close(fd);
	return res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	int fd;
	int res;

	(void) fi;
	if(fi == NULL)
		fd = open(path, O_WRONLY);
	else
		fd = fi->fh;
	
	if (fd == -1)
		return -errno;

	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	if(fi == NULL)
		close(fd);
	return res;
}

static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
	int res;
	int tried = 0;

retry:
	res = statvfs(path, stbuf);
	if (res == -1) {
		if (!tried && !do_fs_fault("statfs", path, 1)) {
			tried = 1;
			goto retry;
		}
		return -errno;
	}

	return 0;
}

static int xmp_release(const char *path, struct fuse_file_info *fi)
{
	(void) path;
	close(fi->fh);
	return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}

#ifdef HAVE_POSIX_FALLOCATE
static int xmp_fallocate(const char *path, int mode,
			off_t offset, off_t length, struct fuse_file_info *fi)
{
	int fd;
	int res;

	(void) fi;

	if (mode)
		return -EOPNOTSUPP;

	if(fi == NULL)
		fd = open(path, O_WRONLY);
	else
		fd = fi->fh;
	
	if (fd == -1)
		return -errno;

	res = -posix_fallocate(fd, offset, length);

	if(fi == NULL)
		close(fd);
	return res;
}
#endif

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int xmp_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
	int res;
	int tried = 0;

retry:
	res = lsetxattr(path, name, value, size, flags);
	if (res == -1) {
		if (!tried && !do_fs_fault("setxattr", path, 0)) {
			tried = 1;
			goto retry;
		}
		return -errno;
	}
	return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value,
			size_t size)
{ 
	int res;
	int tried = 0;
    if (path_isdir(path)) {
        errno = ENOENT;
        (void)do_fs_fault("getxattr", path, 0);
        tried = 1;
    }

retry:
	res = lgetxattr(path, name, value, size);
	if (res == -1) {
		if (!tried && !do_fs_fault("getxattr", path, 0)) {
			tried = 1;
			goto retry;
		}
		return -errno;
	}
	return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
	int res;
	int tried = 0;
    if (path_isdir(path)) {
        errno = ENOENT;
        (void)do_fs_fault("listxattr", path, 0);
        tried = 1;
    }

retry:
	res = llistxattr(path, list, size);
	if (res == -1) {
		if (!tried && !do_fs_fault("listxattr", path, 0)) {
			tried = 1;
			goto retry;
		}
		return -errno;
	}
	return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
	int res;
	int tried = 0;

retry:
	res = lremovexattr(path, name);
	if (res == -1) {
		if (!tried && !do_fs_fault("listxattr", path, 0)) {
			tried = 1;
			goto retry;
		}
		return -errno;
	}
	return 0;
}
#endif /* HAVE_SETXATTR */

#ifdef HAVE_COPY_FILE_RANGE
static ssize_t xmp_copy_file_range(const char *path_in,
				   struct fuse_file_info *fi_in,
				   off_t offset_in, const char *path_out,
				   struct fuse_file_info *fi_out,
				   off_t offset_out, size_t len, int flags)
{
	int fd_in, fd_out;
	ssize_t res;

	if(fi_in == NULL)
		fd_in = open(path_in, O_RDONLY);
	else
		fd_in = fi_in->fh;

	if (fd_in == -1)
		return -errno;

	if(fi_out == NULL)
		fd_out = open(path_out, O_WRONLY);
	else
		fd_out = fi_out->fh;

	if (fd_out == -1) {
		close(fd_in);
		return -errno;
	}

	res = copy_file_range(fd_in, &offset_in, fd_out, &offset_out, len,
			      flags);
	if (res == -1)
		res = -errno;

	close(fd_in);
	close(fd_out);

	return res;
}
#endif

static off_t xmp_lseek(const char *path, off_t off, int whence, struct fuse_file_info *fi)
{
	int fd;
	off_t res;

	if (fi == NULL)
		fd = open(path, O_RDONLY);
	else
		fd = fi->fh;

	if (fd == -1)
		return -errno;

	res = lseek(fd, off, whence);
	if (res == -1)
		res = -errno;

	if (fi == NULL)
		close(fd);
	return res;
}

static const struct fuse_operations xmp_oper = {
	.init           = xmp_init,
	.getattr	= xmp_getattr,
	.access		= xmp_access,
	.readlink	= xmp_readlink,
	.readdir	= xmp_readdir,
	.mknod		= xmp_mknod,
	.mkdir		= xmp_mkdir,
	.symlink	= xmp_symlink,
	.unlink		= xmp_unlink,
	.rmdir		= xmp_rmdir,
	.rename		= xmp_rename,
	.link		= xmp_link,
	.chmod		= xmp_chmod,
	.chown		= xmp_chown,
	.truncate	= xmp_truncate,
#ifdef HAVE_UTIMENSAT
	.utimens	= xmp_utimens,
#endif
	.open		= xmp_open,
	.create 	= xmp_create,
	.read		= xmp_read,
	.write		= xmp_write,
	.statfs		= xmp_statfs,
	.release	= xmp_release,
	.fsync		= xmp_fsync,
#ifdef HAVE_POSIX_FALLOCATE
	.fallocate	= xmp_fallocate,
#endif
#ifdef HAVE_SETXATTR
	.setxattr	= xmp_setxattr,
	.getxattr	= xmp_getxattr,
	.listxattr	= xmp_listxattr,
	.removexattr	= xmp_removexattr,
#endif
#ifdef HAVE_COPY_FILE_RANGE
	.copy_file_range = xmp_copy_file_range,
#endif
	.lseek		= xmp_lseek,
};

int passthrough_main(int argc, char *argv[], int (*pfn_req)(void *, char *), void *sock)
{
	umask(0);
	get_pass_subdir(argc, argv);
    if (pfn_req)
        g_pfn_req = pfn_req;
    g_sock = sock;
	return fuse_main(argc, argv, &xmp_oper, NULL);
}
