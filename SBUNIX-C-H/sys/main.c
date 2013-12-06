#include <defs.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/gdt.h>
#include <sys/idt.h>
#include <sys/irq.h>
#include <sys/kb.h>
#include <sys/timer.h>
#include <sys/mm/regions.h>
#include <sys/mm/mmgr.h>
#include <sys/mm/vmmgr_virtual.h>
#include <sys/kthread.h>
#include <elf.h>
#include <fs.h>
/*
+defs.h - Included typedefs for datatypes
*/
#include <defs.h>

extern bool get_paging_status();
extern void vmmgr_page_allocator_init();
bool is_scheduler_on = 0;
void* kphysfree = 0;
extern void switch_to_user();
extern void do_exec(char*);
extern void tarfs_dir();
extern void init_tarfs();
extern void init_ahci();
extern int get_sb();
extern void update_structures();

void hello_in_user_mode(){
}

void set_kernel_stack(uint64_t stack) //this will update the ESP0 stack used when an interrupt occurs
{
   tss.rsp0 = stack;
}

void start(uint16_t arg)
{
          init_tarfs();
//============ CODE FOR FILE SYSTEM ===============
       //__attribute__((unused))struct test t;
      init_ahci();
      set_sb();
      update_structures();
      get_sb();
      tarfs_dir();
      do_exec("bin/shell");
      while(TRUE)
	      Yield();
       // while(1);
}

#define INITIAL_STACK_SIZE 4096
char stack[INITIAL_STACK_SIZE];
uint32_t* loader_stack;
extern char kernmem, physbase;
struct tss_t tss;

void boot(void)
{
	// note: function changes rsp, local stack variables can't be practically used
	register char *temp1, *temp2;
        uint32_t* modulep;
        void* physfree;
	__asm__(
		"movq %%rsp, %0;"
		"movq %1, %%rsp;"
		:"=g"(loader_stack)
		:"r"(&stack[INITIAL_STACK_SIZE])
	);
	setup_tss();
	reload_gdt();
        reload_idt();
        irq_install();
        timer_install();
        keyboard_install();
        __asm__("sti");
        modulep = (uint32_t*)((char*)(uint64_t)loader_stack[3] + (uint64_t)&kernmem - (uint64_t)&physbase);
        physfree = (void*)(uint64_t)loader_stack[4];
        kphysfree = physfree;
        mm_phy_init(modulep);
        vmmgr_init();
        vmmgr_page_allocator_init();
        cls();
        scheduler_init();
	/*start(
		(uint32_t*)((char*)(uint64_t)loader_stack[3] + (uint64_t)&kernmem - (uint64_t)&physbase),
		&physbase,
		(void*)(uint64_t)loader_stack[4]
	);*/
	for(
		temp1 = "!!!!! start() returned !!!!!", temp2 = (char*)0xb8000;
		*temp1;
		temp1 += 1, temp2 += 2
	) *temp2 = *temp1;
	while(1);
}
