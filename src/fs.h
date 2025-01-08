#ifndef _FS_H
#define _FS_H

#include "types.h"
#include <stddef.h>

#define SB_MAX_NAME 20
#define BLOCK_SIZE  4096
#define INODE_RATIO 16384
#define NDIRECT 15
#define SB_OFFSET 1024
#define MAX_DIRNAME 128

#define DIV_CEIL(a, b) ((a) % (b) ? ((a) / (b)) + 1 : (a) / (b))

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
  u64 block_bitmap; // block_bitmap start
  u64 inode_bitmap; // inode_bitmap start
  u64 inode_table_start;
  u64 block_start;
  u64 free_blocks_count;
  u64 free_inodes_count;
};

int group_desc_init(struct superblock*, struct group_desc*);
void group_desc_print(struct group_desc*);

// types
#define INODE_DIR 1
#define INODE_FILE 2

// modes
#define INODE_READ (1 << 2)
#define INODE_WRITE (1 << 1)
#define INODE_EXEC (1 << 0)

struct dinode
{
  inode_mode_t mode;
  inode_type type;

  u64 created_at;
  u64 last_modified;
  u64 last_accessed;

  u64 blocks[NDIRECT+1];
  u64 blocks_count;
  u64 size;

  u64 parent_inode;
};

#define IBLOCK(gdi, ino)\
	((ino * sizeof(struct dinode)) / BLOCK_SIZE) + gdi->inode_table_start

/* maps a name to a inode */
struct dirent {
	char name[MAX_DIRNAME];
	u64 inode; // absolute inode number
	u8 taken;
};

int dirent_alloc(struct dirent *dirents, struct dirent **dirent);
void dirent_dealloc(struct dirent *dirent);

#endif /* _FS_H */
