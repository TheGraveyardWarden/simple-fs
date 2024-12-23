#ifndef _FS_H
#define _FS_H

#include "types.h"
#include <stddef.h>

#define SB_MAX_NAME 20
#define BLOCK_SIZE  4096
#define INODE_RATIO 16384

struct superblock
{
  u16 magic;
  char name[SB_MAX_NAME];
  u64 inodes_count;
  u64 blocks_count;
  u64 block_groups_count;
  u64 blocks_per_group;
  u64 inodes_per_group;
  u64 mkfs_time;

  u64 block_bitmap_start;
  u64 inode_bitmap_start;
  u64 inode_table_start;
  u64 block_start; // data blocks
};

void sb_init(struct superblock*, const char*, size_t);
void sb_print(struct superblock*);

struct group_desc
{
  u64 inodes_count;
  u64 blocks_count;
  u64 block_bitmap;
  u64 inode_bitmap;
  u64 inode_table_start;
  u64 block_start;
  u64 free_blocks_count;
  u64 free_inodes_count;
};

int group_desc_init(struct superblock*, struct group_desc*);
void group_desc_print(struct group_desc*);

struct dinode
{
  char dummy[256];
};

#endif /* _FS_H */
