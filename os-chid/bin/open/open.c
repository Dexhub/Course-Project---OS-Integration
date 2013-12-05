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
//      read(buf,fd,cnt);
//      printf("\nAfter seek= %d; File contents:%s",seek,buf);
//
//      if((ret = seek(fd, 3)) != 0)
//          printf("seek failure\n");
////      printf("fd = %d\n",fd);
////      printf("Buffer = %s\n",buf);
//      read(buf,fd,cnt);
//      printf("Buffer = %s\n",buf);
//      // Close the file.
//      close(fd);
  }

  return 0;
}
