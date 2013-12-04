#include <stdio.h>
#include <ustdlib.h>

int main(){
  char buf[1000];
  int fd1;
  fd1 = create("haaa.txt\0");
      printf("fd1 = %d\n",fd1);
  //int fd = open("bin/world");
  if( fd1 == 0 )
      printf("File not found!\n");
  else{
      // Read from the file specified by the FD.
      read(buf,fd1);
//      printf("fd = %d\n",fd1);
//      printf("Buffer = %s\n",buf);
      write("===HHHHHHHHHHHHHHH\0",fd1);

//      printf("fd = %d\n",fd1);
//      printf("Buffer = %s\n",buf);
      read(buf,fd1);
      // Close the file.
      //close(fd);
  }

  return 0;
}
