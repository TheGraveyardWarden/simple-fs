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

  read_inode(0, &inode);
  if (!inode_is_root(&inode)) {
	if (create_root_dir() < 0 ) {
		printf("corrupted fs\n");
		exit(1);
	}
	read_inode(0, &inode);
  }

  strncpy(env.cwd, "/\0", 2);
  env.cwd_ino = 0;
  shell.stop = 0;
  shell.env = &env;

  shell_enter(&shell);
}

// TODO:
// remove is buggy.

/*
int main() {
  struct dev dev;
  struct superblock sb;

  if ((dev.fd = open("./fs", O_RDWR)) < 0)
  {
    perror("open()");
    exit(1);
  }

  if (lseek(dev.fd, 1024, SEEK_SET) < 0)
  {
    perror("lseek()");
    exit(1);
  }

  if (read(dev.fd, &sb, sizeof(struct superblock)) < sizeof(struct superblock))
  {
    perror("read()");
    exit(1);
  }

  inode_alloc(&dev, &sb);

  return 0;
}
*/

