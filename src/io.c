#include "io.h"
#include "fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "core.h"
#include "string.h"
#include <errno.h>

extern struct superblock *sbp;
extern struct group_desc *gd;
extern struct dev dev;

int read_block(u64 block_no, u8 *buff)
{
	LOG("read_block(%d, %ld, %p)\n", dev.fd, block_no, buff);

	int nread = 0;

	if (!buff) {
		errno = EINVAL;
		return -1;
	}

	if (lseek(dev.fd, BLOCK_SIZE * block_no, SEEK_SET) < 0)
		return -1;

	if ((nread = read(dev.fd, buff, BLOCK_SIZE)) < 1)
		return -1;

	return nread;
}

int write_block(u64 block_no, u8 *buff)
{
	LOG("write_block(%d, %ld, %p)\n", dev.fd, block_no, buff);

	int nwrite = 0;

	if (!buff) {
		errno = -EINVAL;
		return -1;
	}

	if (lseek(dev.fd, BLOCK_SIZE * block_no, SEEK_SET) < 0)
		return -1;

	if ((nwrite = write(dev.fd, buff, BLOCK_SIZE)) < 1)
		return -1;

	return nwrite;
}

struct superblock* read_superblock() {
	struct superblock *_sbp;
	u8 blk[BLOCK_SIZE];
	u16 i;
	u8 *tmp;

	if (read_block(0, blk) < 0)
		return NULL;
	
	if ((_sbp = malloc(sizeof(struct superblock))) == NULL)
		return NULL;

	for (i = SB_OFFSET, tmp = (u8*)_sbp; i < (sizeof(struct superblock) + SB_OFFSET); i++, tmp++)
		*tmp = blk[i];

	return _sbp;
}

struct group_desc* read_group_desc_table() {
	struct group_desc* _gd;
	u8 blk[BLOCK_SIZE];
	u8 gdi, *tmp;
	int i;

	if (read_block(1, blk) < 0)
		return NULL;

	if ((_gd = malloc(sizeof(struct group_desc) * sbp->block_groups_count)) == NULL)
		return NULL;

	for (gdi = 0, tmp = blk; gdi < sbp->block_groups_count; gdi++, tmp += sizeof(struct group_desc))
		memcpy(_gd+gdi, tmp, sizeof(struct group_desc));

	return _gd;
}

int write_group_desc_table() {
	if (write_block(1, (u8*)gd) < 0)
		return -1;

	return 0;
}

