#include <ustdlib.h>
#include <stdarg.h>
#include <defs.h>

int create( char* buf){
  int cnt = -1; // fd
  __asm__("movq $0x17, %rax;\n\tint $0x80;");
 __asm__ __volatile__("movl %%eax, %0;"
      :"=r"(cnt)
        ://No input operands
      :"%eax"
     );
  return cnt;
}

