#include <stdio.h>
#include <ustdlib.h>

int main(){
  char buf[1000];
  int fd;
  int seek_inc = 1;
  int cnt = 1;
  fd = create("New file.txt\0");
  if( fd == -1 )
      printf("Cannot create file!\n");
  else{
      // Read from the file specified by the FD.
      write("123456789\0",fd);
      read(buf,fd,cnt);
      seek(fd,seek_inc);
      read(buf,fd,cnt);
      seek(fd,seek_inc);
      read(buf,fd,cnt);
      seek(fd,seek_inc);
      read(buf,fd,cnt);
  }

char buf1[1000];
  int fd1 = open("bin/open");
  if( fd1 == 0 )
      printf("File not found!\n");
  else{
      // Read from the file specified by the FD.
      read(buf1,fd1,10);
      printf("Tarfs file content\n");
      printf("fd = %d",fd1);
      printf("\nBuffer = %s\n",buf1);
      // Close the file.
      close(fd);
   }
  return 0;
}
