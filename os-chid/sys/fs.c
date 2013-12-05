/*
=========================FILE SYSTEM STRUCTURE============================
--------------------------------------------------------------------------
SECTOR 0    | SECTOR 1 & 2      | SECTOR 3,4 AND 5 | SECTOR 6 ONWARDS
SUPERBLOCK  | BITMAP FOR BLOCKS | ARRAY OF INODES  | FILES AND DIRECTORIES
--------------------------------------------------------------------------
As of now, the file descriptor is the same as the index of the inode.
Also, every file can use just one block. This is due to a bug that we
havn't been able to solve; Writing one sector clears the values in the
previous sectors

*/

#include <fs.h>
#include <sys/fs_mmgr.h>
#define INODE_COUNT 100
#define BITMAP_COUNT (1100*8) //Bit map count
#define TRUE 1
#define FALSE 0
#define INFLATE_CONSTANT 100 // all the file descritors of the disk will be greater than 100 and of tarfs will be lesser than 100.
                             // Static Smart Hack - smart is relative lol
extern char* read_disk(int);
extern char* write_disk(int,char*);
extern uint64_t file_mmgr_memory_map[];
extern char* read_disk(int);
extern int append_disk(char*,int);

// --- Helper functions-----
int inflate(int fd) //increase the file descriptor number by the inflate constant
{
 // printf("\nInflating descriptor from %d to %d",fd,fd+INFLATE_CONSTANT);
  return (fd+INFLATE_CONSTANT);
}

int deflate(int fd) //decrease the file descriptor number by the inflate constant
{
 // printf("\nDeflating descriptor from %d to %d",fd,fd-INFLATE_CONSTANT);
  return (fd-INFLATE_CONSTANT);
}

// ----  functions to write to handle disk metadata --------

int write_disk_superblock(DWORD offset, char* buf) // Function to write the superblock to the disk
{
  char* write_str = buf;
  char *write_buffer = (char*) ((uint64_t)pages_for_ahci_start_virtual+ 5*(VIRT_PAGE_SIZE));
  //printf("%p write buf\n",write_buffer);
  memcpy(write_buffer,write_str,(sizeof(struct super_block)));

  uint64_t write_buffer_physical =(uint64_t)(((uint64_t)pages_for_ahci_start_virtual+ 5*(VIRT_PAGE_SIZE) - (uint64_t)0xFFFFFFFF80000000));
  //printf("\n Writing : %s at location: %d", write_buffer,offset);
  return write_interface(&abar->ports[0], offset, 0, 1, write_buffer_physical);
}


int write_disk_memory_map(DWORD offset, char* buf)// Function to write the memory map to the disk
{
  char* write_str = buf;
  char *write_buffer = (char*) ((uint64_t)pages_for_ahci_start_virtual+ 5*(VIRT_PAGE_SIZE));
  //printf("%p write buf\n",write_buffer);
  memcpy(write_buffer,write_str,BITMAP_COUNT);

  uint64_t write_buffer_physical =(uint64_t)(((uint64_t)pages_for_ahci_start_virtual+ 5*(VIRT_PAGE_SIZE) - (uint64_t)0xFFFFFFFF80000000));
  //printf("\n Writing : %s at location: %d", write_buffer,offset);
  return write_interface(&abar->ports[0], offset, 0, 1, write_buffer_physical);
}


int write_disk_inode(DWORD offset, char* buf)// Function to write the inodes to the disk
{
  char *write_str = buf;
  char *write_buffer = (char*) ((uint64_t)pages_for_ahci_start_virtual+ 5*(VIRT_PAGE_SIZE));
  memcpy(write_buffer,write_str,sizeof(inode_list));

  uint64_t write_buffer_physical =(uint64_t)(((uint64_t)pages_for_ahci_start_virtual+ 5*(VIRT_PAGE_SIZE) - (uint64_t)0xFFFFFFFF80000000));
  return write_interface(&abar->ports[0], offset, 0, 1, write_buffer_physical);
}


void update_structures() // Update the meta data of the disk in memory
{
  struct super_block* sbp = (struct super_block*)read_disk(0);
  uint64_t bitmap_offset = sbp->start_bitmap;
  uint64_t node_offset = sbp->start_inodes;

  memcpy ((char*)&sb , (const char*) sbp, sizeof(struct super_block));

  uint64_t* d = (uint64_t*)read_disk(bitmap_offset);
  memcpy ((char*)file_mmgr_memory_map , (char*)d,BITMAP_COUNT);

  struct inodes *d1 = (struct inodes*)read_disk(node_offset);
  memcpy ((char*)inode_list, (char*) d1,sizeof(inode_list));
  return;
}

// Update the meta data on the disk from the values in memory
void rsync()
{
  write_disk_superblock(0,(char*)&sb);
  write_disk_memory_map(1,(char*)file_mmgr_memory_map);
  write_disk_inode(3,(char*)inode_list);

  struct super_block* sbp = (struct super_block*)read_disk(0);
  uint64_t bitmap_offset = sbp->start_bitmap;
  uint64_t node_offset = sbp->start_inodes;

  memcpy ((char*)&sb , (const char*) sbp, sizeof(struct super_block));

  uint64_t* d = (uint64_t*)read_disk(bitmap_offset);
  memcpy ((char*)file_mmgr_memory_map , (char*)d,BITMAP_COUNT);

  struct inodes *d1 = (struct inodes*)read_disk(node_offset);
  memcpy ((char*)inode_list, (char*) d1,sizeof(inode_list));
  return;
}

// Initialize the system
void create_superblock()
{
  struct super_block* sbp;
  sbp = &sb;
  sbp->no_blocks = 65536;
  sbp->no_inodes = 124;
  sbp->start_bitmap = 1; // Offset of the bit map sector
  sbp->start_inodes = 3; // Offset of the inode sector
  sbp->start_blocks = 6; // Offset of the block sector

  memset(file_mmgr_memory_map,0,BITMAP_COUNT);
  memset(inode_list,0, sizeof(inode_list));

  return;

}


// === Helper functions =====
int get_free_inode()
{
  int i;
  for(i=0; i<INODE_COUNT; i++)
  {
    if(inode_list[i].usage != TRUE)
     {
       inode_list[i].usage = TRUE;
       return i;
     }
  }
  printf("\n Error No free inode available");
  return -1;
}

int get_free_block()
{
  int block = file_mmgr_alloc_block();
  if (block == -1)
    printf("\nUnable to get free block");
  //rsync();
  return block;
}

//Search a file inode based on the name
int find_file(char *name)
{
  //printf("\n Searching for %s",name);
  int i;
  for(i=0; i<INODE_COUNT; i++)
    if(strcmp(inode_list[i].name,name) == 0 && inode_list[i].type == TYPE_FILE)
      return i;

  return -1;
}

//Search a directory inode based on the name
int find_directory(char *name)
{
  //printf("\n Searching for %s directory",name);
  int i;
  for(i=0; i<INODE_COUNT; i++)
    if(strcmp(inode_list[i].name,name) == 0 && inode_list[i].type == TYPE_DIR)
      return i;

  return -1;
}

// Write contents to disk
void flush_blocks(int block_number, char* contents)
{
  write_disk(block_number,contents);
}

int create_file(char *name)
{
  // Check if the file already exists, in such case, print the message and ask the user to use the write_function to write to the file
  printf("Creating file :%s \n",name);
   if(find_file(name) != -1)
    {
        printf("File with this name exists\n");
        return -1;
    }

  int inode = get_free_inode();
  int sector_block = get_free_block();

  if(inode == -1 || sector_block == -1 )
    {
      printf("Unable to create a file\n");
      return -1;
    }
  strncpy(inode_list[inode].name,name,strlen(name));
  inode_list[inode].start_block = sector_block;
  inode_list[inode].type = TYPE_FILE;

  rsync();
  return inflate(inode);
}

int write_file(char *contents,int inode)
{
  //printf("\nInside write_file");
  inode = deflate(inode);
  if(inode == -1)
    {
      printf("\nUnable to write to the file");
      return -1;
    }
  else
    {
      int size = strlen(contents);
      printf("Writing contents:%s at file descriptor:%d \n",contents,inflate(inode));
      inode_list[inode].size = size;

      rsync();
      flush_blocks(inode_list[inode].start_block, contents);
      return 1;
    }
}

void create_write_file(char *name, char*contents)
{
    write_file(contents,create_file(name));
}

int make_directory(char *name)
{
  printf("\n Creating directory : %s",name);
  if(find_directory(name) != -1)
    {
      printf("\nDirectory with this name exists");
      return -1;
    }
  int inode = get_free_inode();
  int sector_block = 0;

  //  printf("\n using inode:%d and block %d",inode,sector_block);

  strncpy(inode_list[inode].name,name,strlen(name));
  inode_list[inode].size = 0;
  inode_list[inode].start_block = sector_block;
  inode_list[inode].usage = TRUE;
  inode_list[inode].type = TYPE_DIR;

  rsync();
  return inode;
}

int get_file_descriptor(char *name)
{
  return find_file(name);
}

void seek_file(int inode,int position)
{
  inode = deflate(inode);
  printf("seeking by %d position\n",position);
  position = (position + inode_list[inode].position ) % inode_list[inode].size;
  inode_list[inode].position = position;
  return;
}

char* read_file(int inode,int count,char* buf)
{
  inode = deflate(inode);
  if(inode == -1)
    {
      printf(" = No file found");
      return NULL;
    }
  else
    {
      //printf(" = File found!");
      //printf("\n   File name: %s size: %d start_block %d",inode_list[inode].name,inode_list[inode].size,inode_list[inode].start_block);
      char *contents = read_disk(inode_list[inode].start_block);
      int position = inode_list[inode].position;
      while(position != 0)
      {
        contents++;
        position--;
      }
      const char *s='\0';
      //printf("\n Size of buffer:%d",sizeof(buf));
      memcpy(buf,(const char *)&s,sizeof(buf));
      memcpy(buf,contents,count);
      position = inode_list[inode].position;
      //update cursor position in the file and make sure it is within the file size range
      position = (position + count) % inode_list[inode].size;
      inode_list[inode].position = position;
      return buf;
    }
}

char* read_file_by_name(char *name)
{
  int count = 100;
  char buf[100];
  int fd = get_file_descriptor(name);
  char *c = read_file(fd,count,buf);
  //printf("\n FD:%d",fd);
  if (c!= NULL)
    {
     printf("\n File contents:%s",c);
     return c;
    }
  //printf("\nNULL");
  return 0;
}

//creates file statically ; can be used for testing
void create_file_static(){

  char *name = "Hello.txt\0";
  char *contents = "Hello World!!\0";
  create_write_file(name,contents);

  name = "Hello2.txt\0";
  contents = "CLUB 205 Hello World!!\0";
  create_write_file(name,contents);
  name = "Hello267.txt\0";
  create_write_file(name,contents);
  read_file_by_name("Hello.txt\0");
  read_file_by_name("Hello2.txt\0");
  read_file_by_name("Hello267.txt\0");
//
//  name = "Hello3.txt\0";
//  contents = "C3333333333LUB 205 Hello World!!\0";
//  create_file(name,contents);
//
//  name = "Hello4.txt\0";
//  contents = "C444444444444444LUB 205 Hello World!!\0";
//  create_file(name,contents);
//  name = "root_directory\0";
//
//  make_directory(name);
//
}



// LS command for file system
void ls_fs()
{
  printf("-------Disk Contents:-----\n");
  update_structures();
  int inode;
  for(inode=0; inode<INODE_COUNT-95; inode++)
  {
  //  printf("\n Usage_check:%d",inode_list[inode].usage);
  //  printf("\n Usage_check:%d",inode_list[inode].size);
    if(inode_list[inode].usage == TRUE)
      {
        if(inode_list[inode].size == 0)
          printf("| %s\\ \n",inode_list[inode].name);
        else
          printf("| %s \n",inode_list[inode].name);
      }
  }

}

//dummy function to test functionality
int get_sb()
{
 // read_file_by_name("Name ");
//  read_file_by_name("Hello.txt ");
//  read_file_by_name("Hello.txt\0");
 // create_file_static();
 // ls();
  return 1;
}

// Creates a superblock and also sets mmap
int set_sb()
{
  create_superblock();
  rsync();
  int i;
  for (i=0; i<6; i++)
  {
    file_mmgr_set_block(i);
  }
  rsync();

  return 1;
}



