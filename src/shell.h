#ifndef _SHELL_H_
#define _SHELL_H_

#define ENV_CWD_MAX_LEN 256
#define CMD_LEN 20

#include "types.h"
#include "list.h"

struct shell;

typedef int (*cmd_handler_t)(struct shell*, int argc, char *argv[]);

struct cmd {
	char cmd[CMD_LEN];
	cmd_handler_t handler;
	struct list cmds;
};

void cmd_init(struct cmd*, const char*, cmd_handler_t);

struct env
{
  char cwd[ENV_CWD_MAX_LEN];
  u64 cwd_ino;
};

int env_init(struct env*, u64 cwd_ino);

struct shell
{
  struct env *env;
  struct list cmds;
};

void shell_init(struct shell*, struct env*);
int shell_register_cmd(struct shell*, struct cmd*);
int shell_enter(struct shell*);

#endif
