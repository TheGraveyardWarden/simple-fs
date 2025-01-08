#include "ops.h"
#include "file.h"
#include "core.h"

#include <stdio.h>
#include <time.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

int create_root_dir(void)
{
	u64 ino, blkno;
	struct inode inode;

    int err;
	if ((err = inode_alloc(&ino)) < 0)
		return -1;

	if (ino != 0)
		return -2;

	bzero(&inode, sizeof(inode));
	inode.type = INODE_DIR;
	inode.mode = 0b00000111;
	inode.created_at = inode.last_accessed = inode.last_modified = time(NULL);
	inode.parent_inode = 0;

	if ((err = block_alloc(&blkno)) < 0)
		goto inode;

	inode.blocks_count = 1;
	inode.blocks[0] = blkno;

	if (write_inode(ino, &inode) < 0)
		goto block;

	return 0;

block:
	block_dealloc(blkno);
inode:
	inode_dealloc(ino);
	return -1;
}

int create(u64 wd_ino, struct file *file) {
	u64 ino, blkno;
	struct inode inode, wd_inode;
	struct dirent dirent;
	u64 _dummy;

	if (read_inode(wd_ino, &wd_inode) < 0) {
		printf("read_inode()\n");
		return -1;
	}

	int ret;
	if ((ret = dirlookup(wd_ino, file->name, &_dummy)) != -1){
		printf("dirlookup(): %d\n", ret);
		return -1;
	}

	if (inode_alloc(&ino) < 0) {
		printf("inode_alloc()\n");
		return -1;
	}

	bzero(&inode, sizeof(inode));
	inode.type = file->type;
	inode.created_at = inode.last_accessed = inode.last_modified = time(NULL);
	inode.mode = file->mode;
	inode.parent_inode = wd_ino;

	if (block_alloc(&blkno) < 0) {
		printf("block_alloc()\n");
		goto inode;
	}

	inode.blocks_count = 1;
	inode.blocks[0] = blkno;

	if (write_inode(ino, &inode) < 0) {
		printf("write_inode()\n");
		goto block;
	}

	bzero(&dirent, sizeof(struct dirent));
	dirent.inode = ino;
	strncpy(dirent.name, file->name, MAX_DIRNAME);

	if (inode_add_dirent(&wd_inode, &dirent) < 0) {
		printf("inode_add_dirent()\n");
		goto block;
	}

	file->inode = ino;

	return 0;

block:
	block_dealloc(blkno);
inode:
	inode_dealloc(ino);
	return -1;
}

int __inode_is_root(struct inode *inode) {
	if (inode->ino != 0)
		return 0;

	if (inode->type != INODE_DIR)
		return 0;

	return 1;
}


// inode must be INODE_FILE
int __remove_file(struct inode *inode) {
	u64 i;
	struct inode parent_inode;

	if (read_inode(inode->parent_inode, &parent_inode) < 0)
		return -1;

	int ret;
	if ((ret = inode_remove_dirent(&parent_inode, inode->ino)) < 0) {
		printf("inode_remove_dirent: %d\n", ret);
		return -2;
	}

	for (i = 0; i < inode->blocks_count; i++) {
		if (block_dealloc(inode->blocks[i]) < 0)
			return -3;
		printf("deallocated block %ld\n", inode->blocks[i]);
	}

	inode_dealloc(inode->ino);
	return 0;
}

// inode must be INODE_DIR
int __remove_dir(struct inode *inode) {
	u8 data[BLOCK_SIZE];
	struct dirent *dirent;
	u64 i;
	struct inode parent_inode;

	if (read_inode(inode->parent_inode, &parent_inode) < 0)
		return -1;

	int ret;
	if ((ret = inode_remove_dirent(&parent_inode, inode->ino)) < 0) {
		printf("inode_remove_dirent: %d\n", ret);
		return -2;
	}

	if (read_block(inode->blocks[0], data) < 0)
		return -1;

	dirent = (struct dirent*)data;
	for (i = 0; i < MAX_DIR_COUNT; i++, dirent++) {
		if (dirent->taken) {
			remove_inode(dirent->inode);
			dirent->taken = 0;
		}
	}

	block_dealloc(inode->blocks[0]);
	inode_dealloc(inode->ino);

	return 0;
}

int remove_inode(u64 ino) {
	struct inode inode;

	if (read_inode(ino, &inode) < 0) {
		LOG("read_inode()\n");
		return -2;
	}

	if (__inode_is_root(&inode)) {
		LOG("can\'t remove root\n");
		return -1;
	}

	if (inode.type == INODE_DIR) {
		return __remove_dir(&inode);
	} else {
		return __remove_file(&inode);
	}
}

int list(u64 dir_ino, struct file **files, u64 *files_count) {
	struct inode inode, tmp_inode;
	u8 data[BLOCK_SIZE];
	struct dirent *dirent, *trav;
	u64 i, dir_count;
	struct file *file, *file_idx;

	if (read_inode(dir_ino, &inode) < 0) {
		LOG("read_inode()\n");
		return -2;
	}

	if (inode.type != INODE_DIR) {
		LOG("inode not dir\n");
		return -3;
	}

	dir_count = DIR_ENTRY_COUNT(&inode);
	if (dir_count == 0) {
		LOG("dir empty\n");
		return -1;
	}

	if (read_block(inode.blocks[0], data) < 0) {
		LOG("read_block()\n");
		return -2;
	}

	if ((file = malloc(sizeof(struct file) * dir_count)) == NULL) {
		LOG("malloc()\n");
		return -2;
	}

	dirent = (struct dirent*)data;
	for (trav = dirent, i = 0, file_idx = file; i < MAX_DIR_COUNT; trav++, i++, file_idx++) {
		if (!trav->taken)
			continue;

		if (read_inode(trav->inode, &tmp_inode) < 0) {
			free(file);
			return -2;
		}

		file_idx->type = tmp_inode.type;
		file_idx->mode = tmp_inode.mode;
		file_idx->inode = tmp_inode.ino;
		strncpy(file_idx->name, trav->name, MAX_DIRNAME);
	}

	*files = file;
	*files_count = dir_count;

	return 0;
}

int write_bytes(u64 ino, char *bytes, u64 len) {
	struct inode inode;
	u64 more, i, blkno;

	if (read_inode(ino, &inode) < 0)
		return -1;

	if (!(inode.mode & INODE_WRITE)) {
		errno = -EPERM;
		return -2;
	}

	if (inode.type != INODE_FILE) {
		errno = -EINVAL;
		return -2;
	}

	if (len > (inode.blocks_count * BLOCK_SIZE)) {
		more = DIV_CEIL((len - (inode.blocks_count * BLOCK_SIZE)), BLOCK_SIZE);

        if (more + inode.blocks_count >= NDIRECT)
        {
          LOG("write_bytes(): block limit\n");
          return -1;
        }

		for (i = 0; i < more; i++) {
			if (block_alloc(&blkno) < 0)
				return -1;

			inode.blocks[inode.blocks_count+i] = blkno;
		}

		inode.blocks_count += more;
	}

    u64 tmp = len;
	for (i = 0; i < inode.blocks_count; i++) {
      if (tmp > BLOCK_SIZE)
      {
		write_block(inode.blocks[i], bytes+(i*BLOCK_SIZE));
        tmp -= BLOCK_SIZE;
      }
      else
      {
        char cp[BLOCK_SIZE] = {0};
        memcpy(cp, bytes+(i*BLOCK_SIZE), tmp);
        write_block(inode.blocks[i], cp);
        break;
      }
	}

	inode.size = len;
	inode.last_modified = inode.last_accessed = time(NULL);
	write_inode(ino, &inode);

	return 0;
}

int read_bytes(u64 ino, char **bytes, u64 *len) {
	struct inode inode;
	u64 i, tmp;
	u8 data[BLOCK_SIZE];

	if (read_inode(ino, &inode) < 0)
		return -1;

	if (!(inode.mode & INODE_READ)) {
		errno = -EPERM;
		return -2;
	}

	if (inode.type != INODE_FILE) {
		errno = -EINVAL;
		return -2;
	}

	if (inode.blocks_count == 0)
		return -1;

	*len = inode.size;
	*bytes = malloc(*len);

	tmp = inode.size;
	for (i = 0; i < inode.blocks_count; i++) {
		if (read_block(inode.blocks[i], data) < 0)
			return -1;

		if (tmp > BLOCK_SIZE) {
			memcpy((*bytes)+(i*BLOCK_SIZE), data, BLOCK_SIZE);
			tmp -= BLOCK_SIZE;
		} else {
			memcpy((*bytes)+(i*BLOCK_SIZE), data, tmp);
			tmp -= tmp;
			break;
		}
	}

	return 0;
}

void file_print(struct file *file)
{
  printf("%c%c%c%c\t%ld\t%s\n", file->type == INODE_DIR ? 'd' : '-', file->mode & 1 << 2 ? 'r' : '-', file->mode & 1 << 1 ? 'w' : '-', file->mode & 1 ? 'x' : '-', file->inode, file->name);
}

int inode_path(struct inode *inode, char **path)
{
  struct inode p_inode = *inode, tmp_inode = *inode;
  u64 parent_ino;
  struct dirent *dirent;
  u8 data[BLOCK_SIZE];
  int i, d = 0, len = 0, size;
  char *tmp = NULL;

  *path = NULL;

  if (inode->ino == 0)
  {
    *path = malloc(2);
    (*path)[0] = '/';
    (*path)[1] = 0;
    return 0;
  }

  while (p_inode.ino != 0)
  {
    parent_ino = p_inode.parent_inode;
    if (read_inode(parent_ino, &p_inode) < 0)
    {
      LOG("inode_path(): read_inode()\n");
      return -1;
    }

    if (read_block(p_inode.blocks[0], data) < 0)
    {
      LOG("inode_path(): read_block()");
      return -1;
    }

    dirent = (struct dirent*)data;
    for (i = 0; i < MAX_DIR_COUNT; i++, dirent++)
    {
      if (dirent->taken && dirent->inode == tmp_inode.ino)
      {
        tmp = realloc(tmp, MAX_DIRNAME * ++d);
        strncpy(tmp+((d-1)*MAX_DIRNAME), dirent->name, MAX_DIRNAME);
        len += strlen(dirent->name);
        tmp_inode = p_inode;
        break;
      }
    }
  }

  *path = malloc(len + d + 1);
  char *_path = *path;

  for (i = d-1; i >= 0; i--)
  {
    size = strlen(tmp+(i*MAX_DIRNAME));
    *_path++ = '/';
    strncpy(_path, tmp+(i*MAX_DIRNAME), size);
    _path += size;
  }

	free(tmp);
  (*path)[len+d] = 0;


  return 0;
}

int ino_path(u64 ino, char **path) {
	struct inode inode;

	if (read_inode(ino, &inode) < 0)
		return -1;

	return inode_path(&inode, path);
}

