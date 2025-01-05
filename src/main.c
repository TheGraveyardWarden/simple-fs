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


  struct file file = {
	.name = "file",
	.type = INODE_FILE,
	.mode = 0b111
  };

  if (create(_ino, &file) < 0) {
	perror("create()");
	exit(1);
  }

/*
  int ret;
  if ((ret = remove_inode(5)) < 0) {
	printf("remove_inode(): %d\n", ret);
	exit(1);
  }
*/
/*
  char *text = "33434wefdfdf";
  printf("strlen(text): %ld\n", strlen(text));
  if (write_bytes(2, text, strlen(text)) < 0) {
	  printf("write_bytes()\n");
	  exit(1);
  }
*/
/*
  struct inode inode2;
  read_inode(4, &inode2);

  printf("inode: \n");
  printf("parent_inode: %ld\n", inode2.parent_inode);
  printf("size: %ld\n", inode2.size);
  printf("blocks_count: %ld\n", inode2.blocks_count);
  for (int z = 0; z < inode2.blocks_count; z++) {
	printf("blockno#1: %ld\n", inode2.blocks[z]);
  }
  */
/*
  char *bytes;
  u64 len;

  read_bytes(2, &bytes, &len);
  printf("read_bytes: %s\n", bytes);
  free(bytes);
*/
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

