#include "file.h"
#include "core.h"
#include "fs.h"
#include "io.h"
#include "stdlib.h"
#include <string.h>

extern struct superblock *sbp;
extern struct group_desc *gd;
extern struct dev dev;

u64 inode_alloc() {
	u64 inode_idx = 0;
	u8 bitmap[BLOCK_SIZE], *curr, i;
	struct group_desc *gdi;

	for (gdi = gd, i = 0; i < sbp->block_groups_count; i++, gdi++) {
		if (gdi->free_inodes_count > 0) {
			goto found;
		}
	}

	exit(1);

found:	
	read_block(gdi->inode_bitmap, bitmap);

	for (curr = &bitmap[0]; curr < bitmap + (BLOCK_SIZE - 1); curr++) {
		if (*curr == 0xff)
			continue;

		for (i = 0; i < 8; i++) {
			if (!(*curr & (1<<i))) {
				inode_idx = ((curr - bitmap) * 8) + i;
				*curr |= 1 << i;
				write_block(gdi->inode_bitmap, bitmap);
				gdi->free_inodes_count--;
				write_group_desc_table();

				return inode_idx + (sbp->inodes_per_group * (gdi - gd));
			}
		}
	}

	exit(1);
}

int inode_dealloc(u64 ino) {
	u8 gd_idx, *tmp;
	u64 gd_ino;
	u8 bitmap[BLOCK_SIZE];
	struct group_desc *gdi;

	gd_idx = ino / sbp->inodes_per_group;
	gd_ino = ino % sbp->inodes_per_group;
	gdi = gd + gd_idx;

	read_block(gdi->inode_bitmap, bitmap);
	tmp = &bitmap[gd_ino / 8];

	if (*tmp & (1 << (gd_ino % 8))) {
		*tmp &= ~(1 << (gd_ino % 8));
		write_block(gdi->inode_bitmap, bitmap);
		gdi->free_inodes_count++;
		write_group_desc_table();

		return 0;
	}

	return -1;
}

void __read_inode(struct group_desc *gdi, u64 ino, struct dinode *dinode) {
	u8 blk[BLOCK_SIZE];
	u64 i;
	const u64 stride = ino * sizeof(struct dinode) % BLOCK_SIZE;

	read_block(IBLOCK(gdi, ino), blk);
	memcpy(dinode, blk+stride, sizeof(struct dinode));
	
	return;
}

void read_inode(u64 ino, struct inode *inode) {
	u8 gd_idx;
	u64 gd_ino;
	struct dinode dinode;
	struct group_desc *gdi;

	if (inode == NULL)
		exit(1);

	gd_idx = ino / sbp->inodes_per_group;
	gd_ino = ino % sbp->inodes_per_group;
	gdi = gd + gd_idx;

	__read_inode(gdi, gd_ino, &dinode);
	
	memcpy(inode, &dinode, sizeof(struct dinode));
	inode->block_group = gd_idx;
}

