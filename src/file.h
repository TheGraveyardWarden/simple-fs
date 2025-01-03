#ifndef _FILE_H
#define _FILE_H

#include "types.h"
#include "fs.h"
#include "io.h"

struct inode {
  inode_mode_t mode;
  u16 uid;
  u16 gid;

  inode_type type;

  u64 created_at;
  u64 last_modified;
  u64 last_accessed;

  u64 blocks[NDIRECT+1];
  u64 blocks_count;
  u64 size;

  // ----- mem barrier ------
  // add new fields belove here

  u64 block_group;
};

int inode_alloc(u64*);
int inode_dealloc(u64);
int __read_inode(struct group_desc*, u64, struct dinode*);
int read_inode(u64, struct inode*);

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
