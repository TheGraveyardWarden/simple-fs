#ifndef _OPS_H
#define _OPS_H

#include "fs.h"
#include "file.h"

struct file {
	char name[MAX_DIRNAME];
	inode_type type;
	inode_mode_t mode;
	u64 inode;
};

void file_print(struct file*);

int create(u64 wd_ino, struct file *file);
int create_root_dir(void);
int list(u64 dir_ino, struct file **files, u64 *files_count);
int remove_inode(u64 ino);
int write_bytes(u64 ino, char *bytes, u64 len);
int read_bytes(u64 ino, char **bytes, u64 *len);
int inode_path(struct inode *inode, char **path);
int ino_path(u64 ino, char **path);

#endif
