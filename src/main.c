#include "core.h"
#include "file.h"
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "fs.h"
#include "io.h"
#include "file.h"
#include "ops.h"
#include "shell.h"

struct superblock *sbp;
struct group_desc *gd;
struct dev 		  dev;

#define CWD shell->env->cwd_ino
#define FILE_DEFAULT_MODE 6

int ls(struct shell* shell, int argc, char *argv[]) {
	struct file *files;
	u64 files_len;
	u64 ino;
	struct inode inode;
	int ret;

	if (argc == 1) {
		ino = CWD;
		goto list;
	}

	if ((ret = pathlookup(CWD, argv[1], &inode)) < 0) {
		printf("file not found\n");
		return ret;
	}

	ino = inode.ino;

list:
	if ((ret = list(ino, &files, &files_len)) < 0) {
		if (ret == -1) {
			printf("directory is empty\n");
			return 0;
		}

		return -2;
	}

	for (int _i = 0; _i < files_len; _i++)
		file_print(&files[_i]);

	free(files);

	return 0;
}

int cd(struct shell* shell, int argc, char *argv[]) {
	struct inode inode;
	char *path;

	if (argc == 1) {
		CWD = ROOT_INO;
		strncpy(shell->env->cwd, "/\0", 2);
		return 0;
	}

	if (pathlookup(CWD, argv[1], &inode) < 0) {
		printf("file not found\n");
		return -1;
	}

	if (inode_path(&inode, &path) < 0)
		return -2;

	strncpy(shell->env->cwd, path, ENV_CWD_MAX_LEN);
	CWD = inode.ino;

	free(path);

	return 0;
}

int touch(struct shell* shell, int argc, char *argv[]) {
	u64 file_dir_ino = CWD;
	struct file file;

	if (argc < 2) {
		printf("too few args\n");
		return -1;
	}

	if (file_init(&file, &file_dir_ino, argv[1], INODE_FILE, FILE_DEFAULT_MODE) < 0)
		return -2;

	if (create(file_dir_ino, &file) < 0)
		return -3;

	return 0;
}

int mkdir(struct shell* shell, int argc, char *argv[]) {
	u64 file_dir_ino = CWD;
	struct file file;

	if (argc < 2) {
		printf("too few args\n");
		return -1;
	}

	if (file_init(&file, &file_dir_ino, argv[1], INODE_DIR, FILE_DEFAULT_MODE) < 0)
		return -2;

	if (create(file_dir_ino, &file) < 0)
		return -3;

	return 0;
}

int rm(struct shell* shell, int argc, char *argv[]) {
	struct inode inode;

	if (argc < 2) {
		printf("too few args\n");
		return -1;	
	}

	if (pathlookup(CWD, argv[1], &inode) < 0) {
		printf("file not found\n");
		return -2;
	}

	if (remove_inode(inode.ino) < 0)
		return -3;

	return 0;
}

int cat(struct shell* shell, int argc, char *argv[]) {
	char *data;
	u64 len, i;
	struct inode inode;

	if (argc < 2) {
		printf("too few args\n");
		return -1;
	}

	if (pathlookup(CWD, argv[1], &inode) < 0) {
		printf("file not found\n");
		return -2;
	}

	if (read_bytes(&inode, &data, &len) < 0) {
		return -3;
	}

	for (i = 0; i < len; i++)
		printf("%c", data[i]);
	printf("\n");

	free(data);

	return 0;
}

int _write(struct shell* shell, int argc, char *argv[]) {
	struct inode inode;

	if (argc < 3) {
		printf("too few args\n");
		return -1;
	}

	if (pathlookup(CWD, argv[1], &inode) < 0) {
		printf("file not found\n");
		return -2;
	}

	if (write_bytes(&inode, argv[2], strlen(argv[2]), 0) < 0) {
		return -3;
	}

	return 0;
}

int append(struct shell* shell, int argc, char *argv[]) {
	struct inode inode;

	if (argc < 3) {
		printf("too few args\n");
		return -1;
	}

	if (pathlookup(CWD, argv[1], &inode) < 0) {
		printf("file not found\n");
		return -2;
	}

	if (write_bytes(&inode, argv[2], strlen(argv[2]), 1) < 0) {
		return -3;
	}

	return 0;
}

int pwd(struct shell* shell, int argc, char *argv[]) {
	printf("%s\n", shell->env->cwd);
	return 0;
}

int load(struct shell* shell, int argc, char *argv[]) {
	int fd;
	struct stat fstat;
	const ssize_t CHUNK_SZ = 1024;
	char data[CHUNK_SZ];
	ssize_t nread;
	struct inode inode;

	if (argc < 3) {
		printf("too few args\nusage: load </path/to/host/file> </path/to/file/in/fs>\n");
		return -1;
	}

	if ((fd = open(argv[1], O_RDONLY)) < 0) {
		perror("open()");
		return -2;
	}

	if (touch(shell, 2, argv+1) < 0)
		return -1;

	if (pathlookup(CWD, argv[2], &inode) < 0) {
		printf("file not found\n");
		return -3;
	}

	while ((nread = read(fd, data, CHUNK_SZ)) > 0) {
		if (write_bytes(&inode, data, nread, 1) < 0) {
			remove_inode(inode.ino);
			printf("failed to write\n");
			return -4;
		}
	}

	close(fd);

	return 0;
}

int dump(struct shell* shell, int argc, char *argv[]) {
	int fd;
	struct inode inode;
	char *data;
	u64 len;

	if (argc < 3) {
		printf("too few args\nusage: dump </path/to/file/in/fs> </path/to/host/file>\n");
		return -1;
	}

	if ((fd = open(argv[2], O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) < 0) {
		perror("open()");
		return -2;
	}

	if (pathlookup(CWD, argv[1], &inode) < 0) {
		printf("file not found\n");
		return -3;
	}

	if (read_bytes(&inode, &data, &len) < 0)
		return -4;

	if (write(fd, data, len) < 0) {
		perror("write()");
		return -5;
	}

	free(data);
	close(fd);

	return 0;
}

int stat(struct shell* shell, int argc, char *argv[]) {
	struct inode inode;

	if (argc < 2) {
		printf("too few args\n");
		return -1;
	}

	if (pathlookup(CWD, argv[1], &inode) < 0) {
		printf("file not found\n");
		return -2;
	}

	printf("inode: %ld\n", inode.ino);
	printf("block group: %ld\n", inode.block_group);
	printf("mode: %c%c%c\n", inode.mode & INODE_READ ? 'r' : '-', inode.mode & INODE_WRITE ? 'w' : '-', inode.mode & INODE_EXEC ? 'x' : '-');
	printf("type: %s\n", inode.type == INODE_DIR ? "directory" : "file");
	printf("created at: %ld\n", inode.created_at);
	printf("last modified: %ld\n", inode.last_modified);
	printf("last accessed: %ld\n", inode.last_accessed);
	printf("blocks count: %ld\n", inode.blocks_count);
	printf("size: %ld\n", inode.size);
	printf("parent inode: %ld\n", inode.parent_inode);

	return 0;
}

int chmod(struct shell* shell, int argc, char *argv[]) {
	struct inode inode;
	inode_mode_t new_mode;

	if (argc < 3) {
		printf("too few args\nusage: chmod </path/to/file> <new-mode>\n");
		return -1;
	}

	if (pathlookup(CWD, argv[1], &inode) < 0) {
		printf("file not found\n");
		return -2;
	}

	new_mode = atoi(argv[2]);

	if (new_mode > 7 && new_mode < 0) {
		printf("invalid mode. mode is a number between 0-7 (3 bit rwx)");
		return -3;
	}

	inode.mode = new_mode;
	
	if (write_inode(inode.ino, &inode) < 0) {
		printf("something went wrong\n");
		return -4;
	}

	return 0;
}

int main() {
  struct inode inode;
  struct env env;
  struct shell shell;
	struct cmd cmd;
  int err;

  if ((dev.fd = open("./fs", O_RDWR)) < 0) {
    perror("open()");
    exit(1);
  }

  sbp = read_superblock(&dev);
  gd = read_group_desc_table(&dev, sbp);

  read_inode(0, &inode);
  if (!inode_is_root(&inode)) {
	if ((err = create_root_dir()) < 0 ) {
		printf("corrupted fs: %d\n", err);
		exit(1);
	}
	read_inode(0, &inode);
  }

	env_init(&env, ROOT_INO);
	shell_init(&shell, &env);

	cmd_init(&cmd, "ls", ls);
	shell_register_cmd(&shell, &cmd);

	cmd_init(&cmd, "cd", cd);
	shell_register_cmd(&shell, &cmd);

	cmd_init(&cmd, "touch", touch);
	shell_register_cmd(&shell, &cmd);

	cmd_init(&cmd, "mkdir", mkdir);
	shell_register_cmd(&shell, &cmd);

	cmd_init(&cmd, "rm", rm);
	shell_register_cmd(&shell, &cmd);

	cmd_init(&cmd, "cat", cat);
	shell_register_cmd(&shell, &cmd);

	cmd_init(&cmd, "write", _write);
	shell_register_cmd(&shell, &cmd);

	cmd_init(&cmd, "append", append);
	shell_register_cmd(&shell, &cmd);

	cmd_init(&cmd, "pwd", pwd);
	shell_register_cmd(&shell, &cmd);

	cmd_init(&cmd, "load", load);
	shell_register_cmd(&shell, &cmd);

	cmd_init(&cmd, "dump", dump);
	shell_register_cmd(&shell, &cmd);

	cmd_init(&cmd, "stat", stat);
	shell_register_cmd(&shell, &cmd);

	cmd_init(&cmd, "chmod", chmod);
	shell_register_cmd(&shell, &cmd);

  shell_enter(&shell);
}

