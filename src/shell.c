#include "shell.h"
#include "core.h"
#include "fs.h"
#include <string.h>
#include "file.h"
#include "ops.h"
#include <stdlib.h>

#define SHELL_CMD_SZ 512

extern struct superblock *sbp;

int shell_enter(struct shell *shell)
{
  char cmd[SHELL_CMD_SZ], *tmp;

  while (!shell->stop)
  {
    printf("[user@%s]-[%s] $ ", sbp->name, shell->env->cwd);
    fflush(stdout);

    if (fgets(cmd, SHELL_CMD_SZ, stdin) == 0)
    {
      LOG("shell_enter(): fgets()\n");
      return -1;
    }
    tmp = strchr(cmd, '\n');
    *tmp = 0;

    if (!strncmp(cmd, "cd ", 3))
    {
      tmp = &cmd[0];
      tmp += 3;
      tmp[ENV_CWD_MAX_LEN - 1] = 0;

      struct inode inode;
      int res;
      if ((res = pathlookup(shell->env->cwd_ino, tmp, &inode)) < 0)
      {
        LOG("shell_enter(): pathlookup(): %d\n", res);
        continue;
      }

      if (inode.type != INODE_DIR)
      {
        printf("path should be a directory\n");
        continue;
      }

      char *path;
      int err;
      if ((err = inode_path(&inode, path)) < 0)
      {
        printf("inode_path(): %d\n", err);
        continue;
      }
      printf("inode_path success: %s\n", path);

      strncpy(shell->env->cwd, tmp, ENV_CWD_MAX_LEN);
      shell->env->cwd_ino = inode.ino;
    }
    else if (!strncmp(cmd, "touch ", 6))
    {
      tmp = &cmd[0];
      tmp += 6;
      tmp[ENV_CWD_MAX_LEN - 1] = 0;

      struct file file;
      file.type = INODE_FILE;
      file.mode = 0b00000111;
      strncpy(file.name, tmp, MAX_DIRNAME);

      if (create(shell->env->cwd_ino, &file) < 0)
      {
        LOG("shell_enter(): create()\n");
        continue;
      }
    }
    else if (!strncmp(cmd, "ls ", 3))
    {
      tmp = &cmd[0];
      tmp += 3;
      tmp[ENV_CWD_MAX_LEN - 1] = 0;

      struct inode inode;
      int res;
      if ((res = pathlookup(shell->env->cwd_ino, tmp, &inode)) < 0)
      {
        LOG("shell_enter(): pathlookup(): %d\n", res);
        continue;
      }

      if (inode.type != INODE_DIR)
      {
        printf("path should be a directory\n");
        continue;
      }

      struct file *files;
      u64 files_len;
      if (list(inode.ino, &files, &files_len) < 0) 
      {
        LOG("shell_enter(): list()\n");
        continue;
      }

      for (int _i = 0; _i < files_len; _i++)
      {
        file_print(&files[_i]);
      }

      free(files);
    }
    else if (!strncmp(cmd, "mkdir ", 6))
    {
      tmp = &cmd[0];
      tmp += 6;
      tmp[ENV_CWD_MAX_LEN - 1] = 0;

      struct file file;
      file.type = INODE_DIR;
      file.mode = 0b00000111;
      strncpy(file.name, tmp, MAX_DIRNAME);

      if (create(shell->env->cwd_ino, &file) < 0)
      {
        printf("mkdir failed\n");
        LOG("shell_enter(): create()\n");
        continue;
      }
    }
    else if (!strncmp(cmd, "pwd\0", 4))
    {
      printf("%s\n", shell->env->cwd);
    }
    else if (!strncmp(cmd, "chmod ", 6))
    {
      tmp = &cmd[0];
      tmp += 6;
      tmp[ENV_CWD_MAX_LEN - 1] = 0;

      char *filename = strchr(tmp, ' ');
      if (!filename)
      {
        printf("bad syntax!\n");
        continue;
      }
      *filename++ = 0;

      int mode = atoi(tmp);
      if (mode > 7 || mode < 0)
      {
        printf("%d is not a valid mode\n", mode);
        continue;
      }

      struct inode inode;
      if (pathlookup(shell->env->cwd_ino, filename, &inode) < 0)
      {
        printf("%s is not a file or directory\n", filename);
        continue;
      }

      inode.mode = mode;

      write_inode(inode.ino, &inode);
    }
    else if (!strncmp(cmd, "cat ", 4))
    {
      tmp = &cmd[0];
      tmp += 4;
      tmp[ENV_CWD_MAX_LEN - 1] = 0;

      printf("cwd_ino: %d\n", shell->env->cwd_ino);

      struct inode inode;
      int err;
      if ((err = pathlookup(shell->env->cwd_ino, tmp, &inode)) < 0)
      {
        printf("pathlookup(): %d\n", err);
        continue;
      }

      printf("%d\n", inode.ino);
    }
    else
    {
      printf("command not found!\n");
    }
  }

  return 0;
}
