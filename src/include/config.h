
#ifndef __CONFIG_H_INCLUDED__
#define __CONFIG_H_INCLUDED__

void config_load(char *path);
char *config_get_server_endpoint(void);
char *config_get_server_mode(void);
char *config_get_server_path(void);
char *config_get_client_endpoint(void);
char *config_get_client_mode(void);
char *config_get_fuse_subdir(void);
char *config_get_fuse_mount(void);
char *config_get_ov_lower(void);
char *config_get_ov_upper(void);
char *config_get_ov_work(void);
char *config_get_ov_mount(void);

#endif
