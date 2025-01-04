#include "fs.h"
#include <string.h>
#include <time.h>
#include <stdio.h>
#include "file.h"

#define GROUP_ITABLE_BLOCK_SIZE(sbp)\
  DIV_CEIL(sizeof(struct dinode) * sbp->inodes_per_group, BLOCK_SIZE)

void sb_init(struct superblock *sb, const char *name, size_t size)
{
  sb->magic = 0xdead;

  strncpy(sb->name, name, SB_MAX_NAME);

  sb->inodes_count = size / INODE_RATIO;
  sb->blocks_count = size / BLOCK_SIZE;
  sb->blocks_per_group = 8 * BLOCK_SIZE;
  sb->block_groups_count = sb->blocks_count / sb->blocks_per_group;
  if (sb->blocks_count % sb->blocks_per_group)
    sb->block_groups_count++;
  sb->inodes_per_group = sb->inodes_count / sb->block_groups_count;

  sb->mkfs_time = time(NULL);

  sb->block_bitmap_start = 2;
  sb->inode_bitmap_start = 2 + sb->block_groups_count;
  sb->inode_table_start = 2 + (2 * sb->block_groups_count);
  sb->block_start = sb->inode_table_start + (GROUP_ITABLE_BLOCK_SIZE(sb) * sb->block_groups_count);
}

void sb_print(struct superblock *sb)
{
  printf("magic: 0x%x\n", sb->magic);
  printf("name: %s\n", sb->name);
  printf("inodes_count: %ld\n", sb->inodes_count);
  printf("blocks_count: %ld\n", sb->blocks_count);
  printf("blocks_per_group: %ld\n", sb->blocks_per_group);
  printf("block_groups_count: %ld\n", sb->block_groups_count);
  printf("inodes_per_group: %ld\n", sb->inodes_per_group);
  printf("mkfs_time: %ld\n", sb->mkfs_time);
  printf("block_bitmap_start: %ld\n", sb->block_bitmap_start);
  printf("inode_bitmap_start: %ld\n", sb->inode_bitmap_start);
  printf("inode_table_start: %ld\n", sb->inode_table_start);
  printf("block_start: %ld\n", sb->block_start);
}

int group_desc_init(struct superblock *sb, struct group_desc *gd)
{
  struct group_desc *gdi;
  size_t i, block_stride;

  block_stride = sb->block_start;
  for (gdi = gd, i = 0; gdi < gd+sb->block_groups_count; gdi++, i++)
  {
    if (gdi == gd+(sb->block_groups_count-1) && sb->blocks_count % sb->blocks_per_group)
      gdi->blocks_count = sb->blocks_count % sb->blocks_per_group;
    else
      gdi->blocks_count = sb->blocks_per_group;

    // exclude blocks containing fs metadata from first block group
    if (gdi == gd)
      gdi->blocks_count -= sb->block_start;

    gdi->inodes_count = sb->inodes_per_group;
    gdi->block_bitmap = sb->block_bitmap_start + i;
    gdi->inode_bitmap = sb->inode_bitmap_start + i;
    gdi->inode_table_start = sb->inode_table_start + (i * GROUP_ITABLE_BLOCK_SIZE(sb));
    gdi->block_start = block_stride;
    block_stride += gdi->blocks_count;

    gdi->free_blocks_count = gdi->blocks_count;
    gdi->free_inodes_count = gdi->inodes_count;
  }
}

void group_desc_print(struct group_desc *gd)
{
  printf("inodes_count: %ld\n", gd->inodes_count);
  printf("blocks_count: %ld\n", gd->blocks_count);
  printf("block_bitmap: %ld\n", gd->block_bitmap);
  printf("inode_bitmap: %ld\n", gd->inode_bitmap);
  printf("inode_table_start: %ld\n", gd->inode_table_start);
  printf("block_start: %ld\n", gd->block_start);
  printf("free_blocks_count: %ld\n", gd->free_blocks_count);
  printf("free_inodes_count: %ld\n", gd->free_inodes_count);
}

int dirent_alloc(struct dirent *dirents, struct dirent **dirent) {
	struct dirent *dir;
	u64 i;

	for (dir = dirents, i = 0; i < MAX_DIR_COUNT; i++, dir++) {
		if (!dir->taken) {
			dir->taken = 1;
			*dirent = dir;
			return 0;
		}
	}

	return -1;
}

void dirent_dealloc(struct dirent *dirent) {
	dirent->taken = 0;
}

