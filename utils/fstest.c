#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "fs.h"

static off_t seek_to_block(int fd, u64 block);

int main()
{
  int fd;
  struct superblock sb;

  if ((fd = open("./fs", O_RDONLY)) < 0)
  {
    perror("open()");
    exit(1);
  }

  if (lseek(fd, 1024, SEEK_SET) < 0)
  {
    perror("lseek()");
    exit(1);
  }

  if (read(fd, &sb, sizeof(struct superblock)) < sizeof(struct superblock))
  {
    perror("read()");
    exit(1);
  }

  sb_print(&sb);

  seek_to_block(fd, 1);

  struct group_desc gdt[sb.block_groups_count], *gdi;
  size_t i;

  if (read(fd, gdt, sizeof(gdt)) < sizeof(gdt))
  {
    perror("read()");
    exit(1);
  }

  for (gdi = gdt, i = 0; gdi < gdt+sb.block_groups_count; gdi++, i++)
  {
    printf("-------- block group #%d --------\n", i);
    group_desc_print(gdi);
  }

  return 0;
}

static off_t seek_to_block(int fd, u64 block)
{
  off_t off;
  if ((off = lseek(fd, block * BLOCK_SIZE, SEEK_SET)) < 0)
  {
    perror("lseek()");
    exit(1);
  }

  return off;
}
