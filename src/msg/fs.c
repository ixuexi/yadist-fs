#include <string.h>
#include "msg.h"

/*
 * argc - arg number of main
 * argv - arg array of main
 * buf  - return the subdir path
 * len  - buf max len
 * return value:
 *  0   - cannot find the subdir arg
 *  > 0 - the length of return path
 */
int get_subdir(int argc, char *argv[], char *buf, int len)
{
    int i, rl;
    for (i = 1; i < argc; i++) 
        if (!strncmp(argv[i], "subdir=", 7)) {
            strncpy(buf, argv[i] + 7, len - 1);
            buf[len - 1] = 0;
            rl = strnlen(buf, len - 1);
            return rl;
        }
    return 0;
}

/*
 *  path    - input file path
 *  dir     - return the file directory
 */
void get_file_dir(char *dir, char *path)
{
    char *cur, *last;
    cur = strstr(path, "/");
    if (!cur)
        return;
    while (cur) {
        last = cur;
        cur = strstr(cur + 1, "/");
    }
    if (dir != path)
        strncpy(dir, path, last - path);
    dir[last - path] = 0;
}

void resolve_path(char path[], char *root, int rlen, char *rela, int len, int type)
{
    if (type == PATH_ABS) {
        strncpy(path, root, rlen);
        memcpy(path + rlen, rela, len);
        path[rlen + len] = 0;
    }
    else if (type == PATH_RELA) {
        strncpy(path, rela, len);
        path[len] = 0;
    }
}

