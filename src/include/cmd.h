
#ifndef __CMD_H_INCLUDED__
#define __CMD_H_INCLUDED__

int option_vec_fill(char *argv[], int size, char *opts);
void option_vec_free(char *argv[], int size);
void option_vec_print(char *argv[], int size);
int execute_command(char *cmd, char *argv[]);

#endif
