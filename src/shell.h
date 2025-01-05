#ifndef _SHELL_H_
#define _SHELL_H_

#define ENV_CWD_MAX_LEN 256

#include "types.h"

struct env
{
  char cwd[ENV_CWD_MAX_LEN];
  u64 cwd_ino;
};

struct shell
{
  u8 stop;
  struct env *env;
};

int shell_enter(struct shell*);

#endif
