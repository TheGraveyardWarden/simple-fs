#include "core.h"
#include "file.h"
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "fs.h"
#include "io.h"
#include "file.h"
#include "ops.h"
#include "shell.h"

struct superblock *sbp;
struct group_desc *gd;
struct dev 		  dev;

#define CWD shell->env->cwd_ino

int ls(struct shell* shell, int argc, char *argv[]) {
	struct file *files;
	u64 files_len;
	u64 ino;
	struct inode inode;
	int ret;

	if (argc == 1) {
		ino = CWD;
		goto list;
	}

	if ((ret = pathlookup(CWD, argv[1], &inode)) < 0)
		return ret;

	ino = inode.ino;

list:
	if (list(ino, &files, &files_len) < 0)
		return -2;

	for (int _i = 0; _i < files_len; _i++)
		file_print(&files[_i]);

	free(files);

	return 0;
}

int cd(struct shell* shell, int argc, char *argv[]) {
	struct inode inode;
	char *path;

	if (argc == 1) {
		CWD = ROOT_INO;
		strncpy(shell->env->cwd, "/\0", 2);
		return 0;
	}

	if (pathlookup(CWD, argv[1], &inode) < 0)
		return -1;

	if (inode_path(&inode, &path) < 0)
		return -2;

	strncpy(shell->env->cwd, path, ENV_CWD_MAX_LEN);
	CWD = inode.ino;

	free(path);

	return 0;
}

int main() {
  struct inode inode;
  struct env env;
  struct shell shell;

  if ((dev.fd = open("./fs", O_RDWR)) < 0) {
    perror("open()");
    exit(1);
  }

  sbp = read_superblock(&dev);
  gd = read_group_desc_table(&dev, sbp);

  int err;
  read_inode(0, &inode);
  if (!inode_is_root(&inode)) {
	if ((err = create_root_dir()) < 0 ) {
		printf("corrupted fs: %d\n", err);
		exit(1);
	}
	read_inode(0, &inode);
  }

	env_init(&env, ROOT_INO);
	shell_init(&shell, &env);

	struct cmd cmd;

	cmd_init(&cmd, "ls", ls);
	shell_register_cmd(&shell, &cmd);

	cmd_init(&cmd, "cd", cd);
	shell_register_cmd(&shell, &cmd);

  shell_enter(&shell);
}

