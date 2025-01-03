#include "file.h"
#include "core.h"
#include "fs.h"
#include "io.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define ABS_INODE(raw_ino, idx, ino)\
	idx = raw_ino / sbp->inodes_per_group;\
	ino = raw_ino % sbp->inodes_per_group;

#define ABS_BLOCK(raw_blkno, idx, blkno)\
	idx = raw_blkno / sbp->blocks_per_group;\
	blkno = raw_blkno % sbp->blocks_per_group;

#define INODE_STRIDE(ino) (ino) * sizeof(struct dinode) % BLOCK_SIZE

extern struct superblock *sbp;
extern struct group_desc *gd;
extern struct dev dev;

int inode_alloc(u64 *inode) {
	u8 bitmap[BLOCK_SIZE], *curr, i;
	struct group_desc *gdi;

	for (gdi = gd, i = 0; i < sbp->block_groups_count; i++, gdi++) {
		if (gdi->free_inodes_count > 0) {
			goto found;
		}
	}

	exit(1);

found:	
	if (read_block(gdi->inode_bitmap, bitmap) < 0)
		return -1;

	for (curr = &bitmap[0]; curr < bitmap + (BLOCK_SIZE - 1); curr++) {
		if (*curr == 0xff)
			continue;

		for (i = 0; i < 8; i++) {
			if (!(*curr & (1<<i))) {
				*inode = ((curr - bitmap) * 8) + i;
				*curr |= 1 << i;

				if (write_block(gdi->inode_bitmap, bitmap) < 0)
					return -1;

				gdi->free_inodes_count--;
				write_group_desc_table(); // no check needed

				*inode += sbp->inodes_per_group * (gdi - gd);
				return 0;
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

	ABS_INODE(ino, gd_idx, gd_ino);
	gdi = gd + gd_idx;

	if (read_block(gdi->inode_bitmap, bitmap) < 0)
		return -1;

	tmp = &bitmap[gd_ino / 8];

	if (*tmp & (1 << (gd_ino % 8))) {
		*tmp &= ~(1 << (gd_ino % 8));
		if (write_block(gdi->inode_bitmap, bitmap) < 0)
			return -1;

		gdi->free_inodes_count++;
		write_group_desc_table(); // no check

		return 0;
	}

	return -1;
}

int __read_inode(struct group_desc *gdi, u64 ino, struct dinode *dinode) {
	u8 blk[BLOCK_SIZE];
	u64 i;
	// const u64 stride = ino * sizeof(struct dinode) % BLOCK_SIZE;
	const u64 stride = INODE_STRIDE(ino);

	if (read_block(IBLOCK(gdi, ino), blk) < 0)
		return -1;

	memcpy(dinode, blk+stride, sizeof(struct dinode));
	
	return 0;
}

int __write_inode(struct group_desc *gdi, u64 ino, const struct dinode *dinode) {
	u8 blk[BLOCK_SIZE];
	const u64 stride = INODE_STRIDE(ino);

	if (dinode == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (read_block(IBLOCK(gdi, ino), blk) < 0)
		return -1;

	memcpy(blk+stride, dinode, sizeof(struct dinode));

	if (write_block(IBLOCK(gdi, ino), blk) < 0)
		return -1;

	return 0;
}

int read_inode(u64 ino, struct inode *inode) {
	u8 gd_idx;
	u64 gd_ino;
	struct dinode dinode;
	struct group_desc *gdi;

	if (inode == NULL) {
		errno = EINVAL;
		return -1;
	}

	ABS_INODE(ino, gd_idx, gd_ino);
	gdi = gd + gd_idx;

	if (__read_inode(gdi, gd_ino, &dinode) < 0)
		return -1;
	
	memcpy(inode, &dinode, sizeof(struct dinode));
	inode->block_group = gd_idx;

	return 0;
}

int write_inode(u64 ino, struct inode* inode) {
	u8 gd_idx;
	u64 gd_ino;
	struct group_desc *gdi;

	ABS_INODE(ino, gd_idx, gd_ino);
	gdi = gd + gd_idx;

	if (__write_inode(gdi, gd_ino, (struct dinode*)inode) < 0)
		return -1;

	return 0;
}

int block_alloc(u64 *blkno) {
	u8 bitmap[BLOCK_SIZE], *curr, i;
	struct group_desc *gdi;

	for (gdi = gd; gdi < gd + (sbp->block_groups_count - 1); gdi++) {
		if (gdi->free_blocks_count > 0)
			goto found;
	}

	errno = ENOMEM;
	return -1;

found:
	// read block bitmap
	if (read_block(gdi->block_bitmap, bitmap) < 0)
		return -1;
	// find first unallocated block

	for (curr = &bitmap[0]; curr < bitmap + (BLOCK_SIZE - 1); curr++) {
		if (*curr == 0xff)
			continue;

		for (i = 0; i < 8; i++) {
			if (!(*curr & (1<<i))) {
				*blkno = ((curr - bitmap) * 8) + i;
				*curr |= 1 << i;

				if (write_block(gdi->block_bitmap, bitmap) < 0)
					return -1;

				gdi->free_blocks_count--;
				write_group_desc_table(); // no check needed

				*blkno += sbp->blocks_per_group * (gdi - gd);
				return 0;
			}
		}
	}
}

int block_dealloc(u64 blkno) {
	u8 gd_idx, *tmp;
	u64 gd_blkno;
	u8 bitmap[BLOCK_SIZE];
	struct group_desc *gdi;

	ABS_BLOCK(blkno, gd_idx, gd_blkno);
	gdi = gd + gd_idx;

	if (read_block(gdi->block_bitmap, bitmap) < 0)
		return -1;

	tmp = &bitmap[gd_blkno / 8];

	if (*tmp & (1 << (gd_blkno % 8))) {
		*tmp &= ~(1 << (gd_blkno % 8));
		if (write_block(gdi->block_bitmap, bitmap) < 0)
			return -1;

		gdi->free_inodes_count++;
		write_group_desc_table(); // no check

		return 0;
	}

	return -1;
}

