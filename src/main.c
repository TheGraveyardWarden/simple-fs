#include "core.h"
#include "file.h"
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include "fs.h"
#include "io.h"


struct superblock *sbp;
struct group_desc *gd;
struct dev 		  dev;

int main() {
  if ((dev.fd = open("./fs", O_RDWR)) < 0) {
    perror("open()");
    exit(1);
  }

  sbp = read_superblock(&dev);
  gd = read_group_desc_table(&dev, sbp);

}

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

