#include <defs.h>
#include <stdio.h>
#include <sys/fs_mmgr.h>
#include <sys/mm/mmgr.h>

/*
Caution getting system specific
*/



/*
_mmnger_memory_map is a pointer to the bit map structure that we use to keep track of all of physical memory.
Each bit is a 0 if that block has not been allocated (Useable) or a 1 if it is reserved (In use).
 The number of bits in this array is _mmngr_max_blocks. In other words, each bit represents a single memory block,
 which in turn, is 4KB of physical memory.

_mmngr_max_blocks containes the amount of memory blocks available. This is the size of physical memory
 (Retrieved from the BIOS from the boot loader) divide by PMMNGR_BLOCK_SIZE. This essentally divides the
 physical address space into memory blocks

_mmngr_used_blocks containes the amount of blocks currently in use

 _mmngr_memory_size is for refrence only--it containes the amount of physical memory in KB
*/


//! number of blocks currently in use
static  uint64_t  file_mmgr_used_blocks=0;

//! maximum number of available memory blocks
static  uint64_t  file_mmgr_max_blocks=65536;

/* memory map bit array. Each bit represents a memory block
Total number of blocks = 127MB / Block_size. i.e 127MB / 4K ~= 32600
No of blocks which can be represented in a byte = 8, because one bit
  represent each block. Thus we need a 32600 / 8 ~= 4100 bit map.
  i.e we need 4100 bytes roughly to represent all the blocks.
On using a 64 bit integer array, each entry in the array can represent
  64 blocks. Thus the size of the bit map array = 4100 / 8 ~= 600.
  Thus I am allocating 600 bytes for the bit map array.
Multiplying by 2 to support 256 MB of RAM
*/

/*
Caution, getting system specific
*/
uint64_t file_mmgr_memory_map[1100];

uint64_t file_mmgr_get_total_blocks(){
        return file_mmgr_max_blocks;
}

uint64_t file_mmgr_get_used_blocks(){
        return file_mmgr_used_blocks;
}

uint64_t file_mmgr_get_total_usable_blocks(){
        return (file_mmgr_get_total_blocks() - file_mmgr_get_used_blocks());
}

/*
To say that the block represented by bit has been used. If bit 47 is passed,
then bit 47 in the 0th index of the array is set.
*/
inline void file_mmgr_set_block (int bit) {
  file_mmgr_memory_map[bit / 64] |= (1 << (bit % 64));
}


/*
Tests if the requested block is free or not.
*/
inline bool file_mmgr_is_block_free (int bit) {
 return file_mmgr_memory_map[bit / 64] &  (1 << (bit % 64));
}

/*
To say that the block represented by bit has is now free to use.
*/
inline void file_mmgr_unset_block (int bit) {
  file_mmgr_memory_map[bit / 64] &= ~ (1 << (bit % 64));
}

void file_mmgr_print_memory_status(){
  printf("Total number of blocks = %d\n",file_mmgr_get_total_blocks());
  printf("Total number of used blocks = %d\n",file_mmgr_get_used_blocks());
  printf("Total number of usable blocks = %d\n\n",file_mmgr_get_total_usable_blocks());
}

/*
Return 1st free block. Note that, each bit in the bit map array will represent a block.
So, say 132 blocks are filled and we are supposed to insert in the the 133rd block.
So it means that memory_map[0] and memory_map[1] will be all F's. memory_map[2] will be
0xFFFFFFFF000011111. Thus for j=5, memory_map[2] & 1<<5, the bit will be 0. Hence, we return
2*64 + 5 = 133rd block as free.
*/
int file_mmgr_get_first_free(){
  uint32_t i,j;
  uint64_t bit;
  for(i=0; i< (file_mmgr_get_total_blocks() / 64); i++){
    if(file_mmgr_memory_map[i] != 0xFFFFFFFFFFFFFFFF){
      for(j=0; j<64; j++){
        bit = 1<<j;
        if( !(file_mmgr_memory_map[i] & bit) )
          return i*64 + j;
      }
    }
  }
  return -1;
}


int file_mmgr_get_first_range_free(int size){
  uint32_t i,j,count;
  uint64_t bit;
  for(i=0; i< (file_mmgr_get_total_blocks() / 64); i++){
      if(file_mmgr_memory_map[i] != 0xFFFFFFFFFFFFFFFF){
          for(j=0; j<64; j++){
              bit = 1<<j;
              if(!(file_mmgr_memory_map[i] & bit)){
                  uint32_t temp_bit = i*64; // Go to that corresponding frame
                  uint32_t free = 0;
                  temp_bit += j;
                  for(count=0; count<size; count++){
                      if( !(file_mmgr_is_block_free(temp_bit + count)) )
                          free++;
                      else{
                          j=j+(count);
                          break; // No use of being in this loop anymore
                      }
                      if(free == size)
                          return ((i*64)+j);  // Found the required range. Return it
                  }
              }
          }
      }
  }
  return -1;
}

/*
Allocates a page from physical memory and returns the address at the physical memory.
Allocates only a single page
*/
int file_mmgr_alloc_block(){
  uint64_t page_frame;
  if(file_mmgr_get_total_usable_blocks() <= 0){
    printf("Total usable blocks is less than or equal to 0\n");
    return -1;
  }

  page_frame = file_mmgr_get_first_free();
  //printf("rpf = %d ",page_frame);
  if(page_frame == -1){
    printf("Not able to find a matching frame\n");
    return -1;
  }

  file_mmgr_set_block(page_frame);
  file_mmgr_used_blocks++;

  //printf("First free page_frame = %d and its address is %p",page_frame,(page_frame * PHY_PAGE_SIZE));
  return (page_frame);
}

void* file_mmgr_alloc_size_blocks(int size){
  uint64_t page_frame;
  int i;
  if(file_mmgr_get_total_usable_blocks() <= size){
    printf("Total usable blocks is less than or equal to 0\n");
    return NULL;
  }

  page_frame = file_mmgr_get_first_range_free(size);
  if(page_frame == -1){
    printf("Not able to find a matching frame\n");
    return NULL;
  }

  for(i=0; i<size; i++){
    file_mmgr_set_block(page_frame + i);
  }

  file_mmgr_used_blocks += size;

  return ((void *)(page_frame * PHY_PAGE_SIZE));
}


void file_mmgr_free_block(void *p){
  int page_frame = (((uint64_t)p)/BLOCK_SIZE);
  if(p == NULL){
    printf("You can't free a NULL pointer!\n");
    return;
  }
  file_mmgr_unset_block(page_frame);
  file_mmgr_used_blocks--;

}

void file_mmgr_free_size_blocks(void *p, int size){
  int page_frame = (((uint64_t)p)/BLOCK_SIZE);
  int i;
  if(p == NULL){
    printf("You can't free a NULL pointer!\n");
    return;
  }
  for(i=0; i<size; i++)
    file_mmgr_unset_block(page_frame + i);
  file_mmgr_used_blocks -= size;

}

void file_mmgr_phy_init(uint32_t* modulep){
  memset(file_mmgr_memory_map,0x0,sizeof(file_mmgr_memory_map));
  return;
}

inline uint16_t file_mmgr_get_block_count(){
  return 0;
}
