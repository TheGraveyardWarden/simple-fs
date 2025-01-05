#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "fs.h"
#include "core.h"


static void usage(const char *prog);
static void usage_create(const char *prog);
static size_t get_size(char* size);
static off_t seek_to_block(int fd, u64 block);

int main(int argc, char *argv[])
{
  int opt;

  if (!argv[optind])
    usage(argv[0]);

  if (!strncmp(argv[1], "create", 6))
  {
    optind++;

    int fd;
    char *fs_name = NULL, *filename;
    size_t file_size, i, nwrite;
    mode_t open_mode;
    struct superblock sb;
    struct group_desc *gdi;
    off_t offset;

    while ((opt = getopt(argc, argv, "n:")) != -1)
    {
      switch(opt)
      {
        case 'n':
          fs_name = optarg;
          break;
        default:
          usage(argv[0]);
      }
    }

    if (argc - optind != 2)
      usage_create(argv[0]);

    filename = argv[optind];
    file_size = get_size(argv[optind+1]);
    open_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;

    if (fs_name == NULL)
      fs_name = filename;

    LOG("open(%s)\n", filename);
    if ((fd = open(filename, O_WRONLY | O_CREAT, open_mode)) < 0)
    {
      perror("open()");
      exit(1);
    }

    LOG("ftruncate(%s, %ld)\n", filename, file_size);
    if (ftruncate(fd, file_size) < 0)
    {
      perror("ftruncate()");
      exit(1);
    }

    sb_init(&sb, fs_name, file_size);
#ifdef DEBUG
    sb_print(&sb);
#endif

    struct group_desc gd[sb.block_groups_count];
    group_desc_init(&sb, gd);
#ifdef DEBUG
    for (gdi = gd, i = 0; gdi < gd+sb.block_groups_count; gdi++, i++)
    {
      LOG("-------- block group #%d --------\n", i);
      group_desc_print(gdi);
    }
#endif

    if ((offset = lseek(fd, 1024, SEEK_SET)) < 0)
    {
      perror("lseek()");
      exit(1);
    }

    if ((nwrite = write(fd, &sb, sizeof(struct superblock))) < sizeof(struct superblock))
    {
      perror("write()");
      exit(1);
    }

    seek_to_block(fd, 1);

    if ((nwrite = write(fd, gd, sizeof(gd))) < sizeof(gd))
    {
      perror("write()");
      exit(1);
    }

    seek_to_block(fd, 2);
    char zero_block[BLOCK_SIZE] = {0};
    for (i = 0; i < sb.block_groups_count*2; i++)
    {
      if (write(fd, zero_block, BLOCK_SIZE) < BLOCK_SIZE)
      {
        perror("write()");
        exit(1);
      }
    }
  } else
  {
    printf("unknown command: %s\n", argv[1]);
    usage(argv[0]);
  }

  return 0;
}

static off_t seek_to_block(int fd, u64 block)
{
  off_t off;
  if ((off = lseek(fd, block * BLOCK_SIZE, SEEK_SET)) < 0)
  {
    perror("lseek()");
    exit(1);
  }

  return off;
}

static size_t get_size(char* size) {
  size_t mul;

  switch (size[strlen(size)-1]) {
    case 'K':
      mul = 1024;
      break;
  case 'M':
      mul = 1024*1024;
      break;
  case 'G':
      mul = 1024*1024*1024;
      break;
  default:
      mul = 1;
      break;
  }

  if (mul != 1)
    size[strlen(size)-1] = 0;

  return atol(size) * mul;
}

static void usage(const char *prog)
{
  printf("usage: %s COMMAND ...\n", prog);
  printf("\nCOMMANDS:\n");
  printf("\tcreate: %s create FILENAME FILE_SIZE [OPTIONS]\n", prog);
  printf("\t\tcreate a simple-fs image\n");
  printf("\t\tFILENAME: image file\n");
  printf("\t\tFILE_SIZE: a number followed by K, M or G for kilobyte, megabyte or gigabyte\n");
  printf("\t\tOPTIONS:\n");
  printf("\t\t\t-n: fs name\n");
  exit(1);
}

static void usage_create(const char *prog)
{
  printf("create: %s create FILENAME FILE_SIZE [OPTIONS]\n", prog);
  printf("\tcreate a simple-fs image\n");
  printf("\tFILENAME: image file\n");
  printf("\tFILE_SIZE: a number followed by K, M or G for kilobyte, megabyte or gigabyte\n");
  printf("\tOPTIONS:\n");
  printf("\t\t-n: fs name\n");
  exit(1);
}
