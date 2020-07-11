#include <limits.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "zmq.h"
#include "czmq.h"

zsock_t *g_agent;
zsock_t *g_server;

char *g_myipaddr;

#define SER_PORT "10000"
#define AGT_PORT "20000"

#define SER_RCV_TIM 5000
#define AGT_RCV_TIM 3000

#define used_free(p) if (p) free(p)

struct result {
    int code;
    char msg[512];
};

struct cmdargs {
    char *cmdname;
    char *workdir;
    char *program;
    char *role;
    char *ipaddr;
};

zlist_t *g_sercmds;
zlist_t *g_agtcmds;

void get_local_ipaddr(void)
{
    ziflist_t *iflist = ziflist_new ();
    
    size_t items = ziflist_size (iflist);
    if (1) {
        printf ("ziflist: interfaces=%zu\n", ziflist_size (iflist));
        const char *name = ziflist_first (iflist);
        while (name) {
            printf (" - name=%s address=%s netmask=%s broadcast=%s\n",
                    name, ziflist_address (iflist), ziflist_netmask (iflist), 
                    ziflist_broadcast (iflist));
            if (strncmp(ziflist_address (iflist), "127", 3))
                if (!g_myipaddr)
                    g_myipaddr = strdup(ziflist_address (iflist));
            name = ziflist_next (iflist);
        }
    }
    ziflist_destroy (&iflist);
}

void nohup_proc(int sig)
{
    (void)sig;
}

void set_nohup(void)
{
    struct sigaction act = {0};
    act.sa_handler = nohup_proc;
    int ret = sigaction(SIGHUP, &act, NULL);
    printf("set nohup ret %d\n", ret);
}

zlist_t *get_cmdlist(char *role)
{
    if (!strcmp(role, "server"))
        return g_sercmds;
    else
        return g_agtcmds;
}

struct cmdargs *get_cmd(char *role, char *cmd)
{
    zlist_t *cmdlist = get_cmdlist(role);
    struct cmdargs *ca = (struct cmdargs *)zlist_first(cmdlist);
    while (ca) {
        if (!strcmp(ca->cmdname, cmd))
            return ca;
        ca = (struct cmdargs *)zlist_next(cmdlist);
    }
    return NULL;
}

void exec_cmd(struct cmdargs *cmd, char *ipaddr, char *arg)
{
    if (cmd) {
        int status;
        char prog[128];
        char args[512];
        char buf[128];
        int pfd[2], len;
        if (pipe(pfd)) {
            printf("create pipe ret %d\n", errno);
            return;
        }
        if (*(cmd->workdir) != 0)
            (void)sprintf(prog, "%s/%s", cmd->workdir, cmd->program);
        else
            strcpy(prog, cmd->program);
        pid_t pid = fork();
        if (!pid) {
            chdir(cmd->workdir);
            //dup2(pfd[1], 0);
            dup2(pfd[1], 1);
            //dup2(pfd[1], 2);
            close(pfd[1]);
            close(pfd[0]);
            (void)execl(prog, prog, cmd->role, ipaddr, arg, NULL);
            _exit(0);
            printf("Exception: out of execl range!\n");
        }
        close(pfd[1]);
        printf("==================cmd-output=================\n");
        do {
            len = read(pfd[0], buf, 127);
            buf[len] = 0;
            printf("%s", buf);
        } while (len != 0);
        printf("==================cmd-output=================\n");
        close(pfd[0]);
        waitpid(pid, &status, 0);
        printf("execl: %s %s %s %s %s ret %d\n", prog, prog, cmd->role, ipaddr, 
            arg, status);
    }
    else
        printf("cmd is null!\n");
}

char *decode_msg(zmsg_t *msg, char *role, char *ip, char *arg, struct result *res)
{
    zframe_t *fr = zmsg_first(msg);
    char *ipaddr = NULL;
    char *target = NULL;
    char *cmd = zframe_strdup(fr);
    struct cmdargs *ca = get_cmd(role, cmd);
    if (ca) {
        fr = zmsg_next(msg);
        ipaddr = zframe_strdup(fr);
        if (ip)
            strcpy(ip, ipaddr);
        fr = zmsg_next(msg);
        target = zframe_strdup(fr);
        if (arg)
            strcpy(arg, target);
        printf("agent recv ip %s target %s\n", ipaddr, target);
        exec_cmd(ca, ipaddr, target);
        res->code = 0;
        strcpy(res->msg, "OK");
    }
    else {
        printf("agent recv invalid command %s\n", cmd);
        res->code = -1;
        strcpy(res->msg, "unknow cmd: ");
        strncat(res->msg, cmd, 100);
    }
    used_free(ipaddr);
    used_free(target);
    return cmd;
}

void agent_decode_msg(zmsg_t *msg, struct result *res)
{
    char *cmd = decode_msg(msg, "agent", NULL, NULL, res);
    used_free(cmd);
}

void rep_loop(const char *role, zsock_t *s, void (*msg_pfn)(zmsg_t *msg, 
    struct result *))
{
    struct result res;
    while (1) {
        zmsg_t *msg = zmsg_recv(s);
        if (!msg) {
            printf("%s recv null msg!\n", role);
            continue;
        }
        msg_pfn(msg, &res);
        zmsg_destroy(&msg);
        msg = zmsg_new();
        zmsg_addstrf(msg, "%s RET %d MSG %s\n", role, res.code, res.msg);
        zmsg_send(&msg, s);
        printf("%s RET %d MSG %s\n", role, res.code, res.msg);
    }
}

void agent_create(char *ip, char *port)
{
    char end[128];
    (void)sprintf(end, "tcp://%s:%s", ip, port);
    g_agent = zsock_new_rep(end);
}

void agent_start(void)
{
    agent_create("0.0.0.0", AGT_PORT);
    printf("start agent on %s\n", AGT_PORT);
    rep_loop("AGENT", g_agent, agent_decode_msg);
}

zmsg_t *req_msg(char *cmd, char *conip, char *target)
{
    zmsg_t *msg = zmsg_new();
    zmsg_addstr(msg, cmd);
    zmsg_addstr(msg, conip);
    zmsg_addstr(msg, target);
    return msg;
}

void send_request(char *toip, char *port, char *cmd, char *myip, char *target, 
    struct result *res, int timeout)
{
    char end[128];
    char *rc;
    int ret;
    (void)sprintf(end, "tcp://%s:%s", toip, port);
    zsock_t *s = zsock_new_req(end);
    if (s) {
        zsock_set_rcvtimeo(s, timeout);
        zmsg_t *msg = req_msg(cmd, myip, target);
        ret = zmsg_send(&msg, s);
        if (ret) {
            zmsg_destroy(&msg);
            (void)sprintf(res->msg, "SEND TO %s RET %d", end, ret);
            res->code = ret;
            printf("%s\n", res->msg);
        }
        msg = zmsg_recv(s);
        if (msg) {
            zframe_t *fr = zmsg_first(msg);
            rc = zframe_strdup(fr);
            res->code = 0;
            strcpy(res->msg, rc);
            free(rc);
            zmsg_destroy(&msg);
            printf("send_request: recv %s\n", res->msg);
        }
        else {
            res->code = -1;
            (void)sprintf(res->msg, "RECV FROM %s TIMEOUT", end);
            printf("%s\n", res->msg);
        }
    }
    else {
        (void)sprintf(res->msg, "SOCK %s CREATE FAILED", end);
        res->code = -1;
        printf("%s\n", res->msg);
    }

req_end:
    if (s)
        zsock_destroy(&s);
}

void server_decode_msg(zmsg_t *msg, struct result *res)
{
    char ipaddr[512];
    char arg[512];
    char *cmd = decode_msg(msg, "server", ipaddr, arg, res);
    send_request(ipaddr, AGT_PORT, cmd, g_myipaddr, arg, res, 
        AGT_RCV_TIM);
    used_free(cmd);
}

void server_create(char *ip, char *port)
{
    char end[128];
    (void)sprintf(end, "tcp://%s:%s", ip, port);
    g_server = zsock_new_rep(end);
}

void server_start(void)
{
    server_create("0.0.0.0", SER_PORT);
    printf("start server on %s\n", SER_PORT);
    rep_loop("SERVER", g_server, server_decode_msg);
}

void run_process(void (*pfun)(void), int daemon)
{
    pid_t pid = 0;

    if (daemon) {
        printf("run in daemon\n");
        pid = fork();
    }
    if (!pid) {
        pfun();
    }
}

void load_byrole(zconfig_t *cfg, char *role, zlist_t *cmds)
{
    zconfig_t *c = zconfig_locate(cfg, role);
    printf("pickup %s cmds\n", role); 
    if (c) {
        zconfig_t *cmd = zconfig_child(c);
        while (cmd) {
            char *cmdname = zconfig_name(cmd);
            char *workdir = zconfig_get(cmd, "/workdir", "");
            char *program = zconfig_get(cmd, "/program", "");
            struct cmdargs *ca = malloc(sizeof(struct cmdargs));
            ca->cmdname = cmdname;
            ca->workdir = workdir;
            ca->program = program;
            ca->role = role;
            ca->ipaddr = NULL;
            printf("cmd %s workdir %s program %s\n", cmdname, workdir, program);
            zlist_append(cmds, ca);
            cmd = zconfig_next(cmd);
        }
    }
}

void load_config(char *prog_path)
{
    char cf[PATH_MAX];
    (void)sprintf(cf, "%s/../conf/yasched.conf", prog_path);
    zconfig_t *cfg = zconfig_load(cf);
    if (!cf) {
        printf("cannot read %s\n", cf);
        return;
    }
    g_sercmds = zlist_new();
    load_byrole(cfg, "server", g_sercmds);
    g_agtcmds = zlist_new();
    load_byrole(cfg, "agent", g_agtcmds);
}

int opt_has_daemon(int argc, char *argv[])
{
    int i;
    for (i = 1; i < argc; i++)
        if (!strcmp(argv[i], "-D"))
            return 1;
    return 0;
}

int main(int argc, char *argv[])
{
    struct result res;
    int d = 0;
    if (argc < 2)
        return 0;

    setlinebuf(stdout);
    set_nohup();
    get_local_ipaddr();
    load_config(dirname(argv[0]));
    d = opt_has_daemon(argc, argv);
    if (!strcmp(argv[1], "server")) {
        run_process(server_start, d);
    }
    else if (!strcmp(argv[1], "agent")) {
        run_process(agent_start, d);
    }
    else if (!strcmp(argv[1], "cmd")) {
        send_request(argv[2], SER_PORT, argv[3], g_myipaddr, argv[4], &res, 
            SER_RCV_TIM);
        printf("send requst ret %d msg %s\n", res.code, res.msg);
    }
    return 0;
}

