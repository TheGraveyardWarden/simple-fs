#ifndef _FILE_H
#define _FILE_H

#include "types.h"
#include "fs.h"
#include "io.h"

#define ROOT_INO 0

struct inode {
  inode_mode_t mode;
  inode_type type;

  u64 created_at;
  u64 last_modified;
  u64 last_accessed;

  u64 blocks[NDIRECT+1];
  u64 blocks_count;
  u64 size;

  u64 parent_inode;

  // ----- mem barrier ------
  // add new fields below here

  u64 block_group;
  u64 ino;
};

int inode_alloc(u64*);
int inode_dealloc(u64);
int __read_inode(struct group_desc*, u64, struct dinode*);
int __write_inode(struct group_desc*, u64, const struct dinode*);
int read_inode(u64, struct inode*);
int write_inode(u64 ino, struct inode* inode);
int inode_is_root(struct inode *inode);
int inode_add_dirent(struct inode *inode, struct dirent *dirent);
int inode_remove_dirent(struct inode *inode, u64 dirent_ino);

int block_alloc(u64*);
int block_dealloc(u64);

// return 0 on success and -1 if not found and < -1 on err
int dirlookup(u64 base_ino, const char *name, u64 *new_ino);
int pathlookup(u64 wd_ino, const char *path, struct inode *inode);

#define ABS_INODE(raw_ino, idx, ino)\
	idx = raw_ino / sbp->inodes_per_group;\
	ino = raw_ino % sbp->inodes_per_group;

#define ABS_BLOCK(raw_blkno, idx, blkno, tmp)\
	for (tmp = gd; (tmp - gd) < sbp->block_groups_count-1; tmp++) {\
		if (raw_blkno >= tmp->block_start && raw_blkno < (tmp+1)->block_start) {\
			idx = tmp - gd;\
			blkno = raw_blkno - tmp->block_start + 1;\
			break;\
		}\
	}\
	if (raw_blkno >= tmp->block_start && raw_blkno - tmp->block_start <= tmp->blocks_count) {\
		idx = tmp - gd;\
		blkno = raw_blkno - tmp->block_start + 1;\
	}

#define BLOCK_TO_ABS(blkno_p, gdi, tmp)\
	*blkno_p += sbp->block_start-1;\
	for (tmp = gd; tmp < gdi; tmp++) {\
		*blkno_p += tmp->blocks_count;\
	}

#define INODE_TO_ABS(inode_p, gdi)\
	*inode_p += sbp->inodes_per_group * (gdi - gd);

#define INODE_STRIDE(ino) (ino) * sizeof(struct dinode) % BLOCK_SIZE

#define DIR_ENTRY_COUNT(inode_p) ((inode_p)->size / sizeof(struct dirent))
#define MAX_DIR_COUNT BLOCK_SIZE / sizeof(struct dirent)

/*
 *
 * touch /path/to/file
 *
 *
 * :::
 * path_lookup (path) -> struct inode
 * inode_alloc (sb) -> struct inode
 * get_inode 
 * block_alloc (sb) -> struct block
 * */

#endif
