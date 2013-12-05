#include <defs.h>

int seek(int fd,int pos){
  int cnt=0;
  __asm__("movq $0x18, %rax;\n\tint $0x80;");
 __asm__ __volatile__("movl %%eax, %0;"
      :"=r"(cnt)
        ://No input operands
      :"%eax"
     );
  return cnt;
}

