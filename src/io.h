#ifndef _IO_H
#define _IO_H

#include "types.h"
#include "fs.h"

struct dev {
	int fd;
};

int read_block(u64, u8*);
int write_block(u64, u8*);
struct superblock* read_superblock();
struct group_desc* read_group_desc_table();
int write_group_desc_table();


#endif
