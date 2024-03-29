#include "syscall.h"
#include "traps.h"

#define SYSCALL(name) \
  .globl name; \
  name: \
    movl $SYS_ ## name, %eax; \
    int $T_SYSCALL; \
    ret

SYSCALL(fork)
SYSCALL(exit)
SYSCALL(wait)
SYSCALL(pipe)
SYSCALL(read)
SYSCALL(write)
SYSCALL(close)
SYSCALL(kill)
SYSCALL(exec)
SYSCALL(open)
SYSCALL(mknod)
SYSCALL(unlink)
SYSCALL(fstat)
SYSCALL(link)
SYSCALL(mkdir)
SYSCALL(chdir)
SYSCALL(dup)
SYSCALL(getpid)
SYSCALL(sbrk)
SYSCALL(sleep)
SYSCALL(uptime)
SYSCALL(addpath)
SYSCALL(delay)
SYSCALL(getparentid)
SYSCALL(getchildrenid)
SYSCALL(gettime)
SYSCALL(recchildren)
SYSCALL(count_num_of_digits)
SYSCALL(change_process_queue)
SYSCALL(assign_tickets)
SYSCALL(assign_srpf_priority)
SYSCALL(print_process_table)