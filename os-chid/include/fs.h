#include <ahci.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/fs_mmgr.h>
#define TYPE_FILE 1
#define TYPE_DIR 0

int set_sb();
int get_sb();
int write_interface(HBA_PORT *port, DWORD startl, DWORD starth, DWORD count, QWORD buf);

struct super_block{
  uint64_t no_blocks;
  uint64_t no_inodes;
  uint64_t start_bitmap;
  uint64_t start_inodes;
  uint64_t start_blocks;
};
struct super_block sb;

struct inodes{
  char name[100];
  uint32_t size;
  uint64_t start_block;
  bool usage;
  bool type; // 0 indicates Directory 1 indicates file
};
struct inodes inode_list[100]; // update the corresponding static values in the program.

