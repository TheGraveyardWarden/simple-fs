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


struct superblock *sbp;
struct group_desc *gd;
struct dev 		  dev;

int main() {
  struct inode inode;

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
 
  u64 _ino = 0;

/*
  struct file file = {
	.name = "ffiii",
	.type = INODE_FILE,
	.mode = 0b111
  };

  if (create(_ino, &file) < 0) {
	perror("create()");
	exit(1);
  }
 */

  int ret;
  if ((ret = remove_inode(2)) < 0) {
	printf("remove_inode(): %d\n", ret);
	exit(1);
  }

/*
  char *text = "this";
  if (write_bytes(2, text, strlen(text)) < 0) {
	  printf("write_bytes()\n");
	  exit(1);
  }
  */

  char *bytes;
  u64 len;

  read_bytes(1, &bytes, &len);
  printf("read_bytes inode %ld: %s\n", 1, bytes);
  free(bytes);
  //read_bytes(2, &bytes, &len);
  //printf("read_bytes inode %ld: %s\n", 2, bytes);

  struct file *files, *_file;
  u64 files_count, i;

  if (list(_ino, &files, &files_count) < 0) {
	perror("list()\n");
	exit(1);
  }

  for (_file = files, i = 0; i < files_count; _file++, i++) {
	printf("name: %s\n", _file->name);
	printf("type: %d\n", _file->type);
	printf("mode: %d\n", _file->mode);
	printf("inode: %ld\n", _file->inode);
	printf("----------\n");
  }
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

