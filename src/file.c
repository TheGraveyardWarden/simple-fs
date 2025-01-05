#include "file.h"
#include "core.h"
#include "fs.h"
#include "io.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

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

	errno = ENOMEM;
	return -1;

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

				INODE_TO_ABS(inode, gdi);
				return 0;
			}
		}
	}

	return -1;
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
	inode->ino = ino;

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
	struct group_desc *gdi, *tmp;

	for (gdi = gd; gdi < gd + (sbp->block_groups_count - 1); gdi++) {
		if (gdi->free_blocks_count > 0)
			goto found;
	}

	errno = ENOMEM;
	return -1;

found:
	if (read_block(gdi->block_bitmap, bitmap) < 0)
		return -1;

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

				BLOCK_TO_ABS(blkno, gdi, tmp);
        bzero(bitmap, sizeof(bitmap));
        write_block(*blkno, bitmap);
				return 0;
			}
		}
	}
}

int block_dealloc(u64 blkno) {
	u8 gd_idx, *tmp;
	u64 gd_blkno;
	u8 bitmap[BLOCK_SIZE];
	struct group_desc *gdi, *gd_tmp;

	ABS_BLOCK(blkno, gd_idx, gd_blkno, gd_tmp);
	gdi = gd + gd_idx;

	if (read_block(gdi->block_bitmap, bitmap) < 0)
		return -1;

	tmp = &bitmap[gd_blkno / 8];

	if (*tmp & (1 << (gd_blkno % 8))) {
		*tmp &= ~(1 << (gd_blkno % 8));
		if (write_block(gdi->block_bitmap, bitmap) < 0)
			return -1;

		gdi->free_blocks_count++;
		write_group_desc_table(); // no check

		return 0;
	}

	return -1;
}

int dirlookup(u64 base_ino, const char *name, u64 *new_ino) {
	struct inode base_inode, new_inode;
	u8 block[BLOCK_SIZE];
	struct dirent *dirent, *trav_dir;
	u64 dir_count, i;

    LOG("dirlookup(%d, %s)\n", base_ino, name);

    if (base_ino == 0 && *name == '/' && *(name+1) == 0)
    {
      *new_ino = 0;
      return 0;
    }

	if (read_inode(base_ino, &base_inode) < 0)
		return -2;

	if (base_inode.type != INODE_DIR)
		return -3;

	if ((dir_count = DIR_ENTRY_COUNT(&base_inode)) == 0)
		return -1;

	if (base_inode.blocks_count == 0)
		return -1;

	if (read_block(base_inode.blocks[0], block) < 0)
		return -6;

	dirent = (struct dirent*)block;
	for (trav_dir = dirent, i = 0; i < MAX_DIR_COUNT; trav_dir++, i++) {
		if (trav_dir->taken && !strncmp(name, trav_dir->name, MAX_DIRNAME))
		{
			*new_ino = trav_dir->inode;
			return 0;
		}
	}

	return -1;
}

int pathlookup(u64 wd_ino, const char *path, struct inode *inode)
{
	char *tmp;
	u64 ino;

    LOG("pathlookup(%d, %s)\n", wd_ino, path);

    if (*path == '/' && *(path+1) == 0)
    {
      if (read_inode(0, inode) < 0)
        return -11;
      return 0;
    }

	if (*path == '/')
    {
      wd_ino = ROOT_INO;
      path++;
    }

	while ((tmp = strchr(path, '/')))
	{
		*tmp++ = 0;

		if (dirlookup(wd_ino, path, &wd_ino) < 0)
			return -1;

		path = tmp;
	}

	if (dirlookup(wd_ino, path, &ino) < 0)
		return -1;

	if (read_inode(ino, inode) < 0)
		return -2;

	return 0;
}

int inode_is_root(struct inode *inode) {
	u8 bitmap[BLOCK_SIZE];

	if (inode->ino != 0)
		return 0;

	if (inode->type != INODE_DIR) // !is_dir
		return 0;

	if (read_block(gd->inode_bitmap, bitmap) < 0) {
		perror("inode_is_root: read_block()");
		exit(1);
	}

	if (!(bitmap[0] & 0x01)) // !is_allocated
		return 0;

	return 1;
}

int inode_add_dirent(struct inode *inode, struct dirent *dirent) {
	u8 data[BLOCK_SIZE];
	struct dirent *dirents, *ddirent;
	u64 dir_count;

	if (inode->type != INODE_DIR) {
		printf("inode is not dir\n");
		errno = -EINVAL;
		return -1;
	}

	if (inode->blocks_count == 0) {
		printf("inode has no blocks\n");
		errno = -ENOMEM;
		return -1;
	}

	dir_count = DIR_ENTRY_COUNT(inode);
	if (dir_count == MAX_DIR_COUNT) {
		printf("inode max dir\n");
		errno = -ENOMEM;
		return -1;
	}

	if (read_block(inode->blocks[0], data) < 0) {
		printf("read_block\n");
		return -2;
	}

	dirents = (struct dirent*)data;

	if (dirent_alloc(dirents, &ddirent) < 0) {
		printf("dirent_alloc()\n");
		return -1;
	}

	LOG("dirent id: %d\n", ddirent - dirents);

	memcpy(ddirent, dirent, sizeof(struct dirent));
	ddirent->taken = 1;

	if (write_block(inode->blocks[0], data) < 0) {
		printf("write_block\n");
		return -2;
	}

	inode->size += sizeof(struct dirent);
	inode->last_accessed = inode->last_modified = time(NULL);

	if (write_inode(inode->ino, inode) < 0) {
		printf("write_inode\n");
		return -2;
	}

	return 0;
}

int inode_remove_dirent(struct inode *inode, u64 dirent_ino) {
	u8 data[BLOCK_SIZE];
	struct dirent *dirent;
	u64 dir_count, i;
	u8 removed;

	dir_count = DIR_ENTRY_COUNT(inode);
	if (dir_count == 0) {
		return -1;
	}

	if (read_block(inode->blocks[0], data) < 0) {
		return -2;
	}

	dirent = (struct dirent*)data;
	removed = 0;
	for (i = 0; i < MAX_DIR_COUNT; i++, dirent++) {
		if (dirent->taken == 1 && dirent->inode == dirent_ino) {
			dirent_dealloc(dirent);
			removed = 1;
			break;
		}
	}

	if (!removed) {
		LOG("inode not found\n");
		return -1;
	}

	if (write_block(inode->blocks[0], data) < 0)
		return -1;

	inode->size -= sizeof(struct dirent);
	inode->last_accessed = inode->last_modified = time(NULL);

	if (write_inode(inode->ino, inode) < 0) {
		printf("write_inode\n");
		return -2;
	}

	return 0;
}

