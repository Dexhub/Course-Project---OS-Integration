#ifndef _FS_MMGR_H
#define _FS_MMGR_H
#include <stdlib.h>

void file_mmgr_free_size_blocks(void*, int);

void file_mmgr_free_block(void *);

void* file_mmgr_alloc_size_blocks(int);

int file_mmgr_get_first_free();

void file_mmgr_set_block (int bit);

int file_mmgr_alloc_block();

#endif
