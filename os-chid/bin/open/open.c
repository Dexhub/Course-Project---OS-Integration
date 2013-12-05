#include <stdio.h>
#include <ustdlib.h>

int main(){
  char buf[1000];
  int fd1;
//  int seek_inc = 3;
 // int ret=0;
  int cnt = 10;
  fd1 = create("New file.txt\0");
      printf("fd1 = %d\n",fd1);
  //int fd = open("bin/world");
  if( fd1 == -1 )
      printf("Cannot create file!\n");
  else{
      // Read from the file specified by the FD.
      write("123456789\0",fd1);
      read(buf,fd1,cnt);
      printf("\nEntire file contents:%s",buf);
      //seek(fd1,seek_inc);
      read(buf,fd1,cnt);
      printf("\nAfter seek= %d; File contents:%s",seek,buf);
//
//      if((ret = seek(fd1, 3)) != 0)
//          printf("seek failure\n");
////      printf("fd = %d\n",fd1);
////      printf("Buffer = %s\n",buf);
//      read(buf,fd1,cnt);
//      printf("Buffer = %s\n",buf);
//      // Close the file.
//      close(fd1);
  }

  return 0;
}
