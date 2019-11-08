#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "path.h"
// #include "uli  b.c"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_addpath(void)
{
  char * newPath;
  volatile int i = 0, j = 0, k = 0;
  volatile char curChar = '1';
  if(argstr(0, &newPath) < 0)
    return -1;
  curChar = newPath[0];
  while(1) {
    curChar = newPath[k++];
    if (i > 9 || curChar == '\0') {
      return 0;
    }
    if (curChar == ':') {
      i++;
      j = 0;
      continue;
    }
    PATH[i][j] = curChar;
    j++; 
  }
  return 0;
}

int
sys_delay(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < 100 * n){
    if(myproc()->killed){
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;

}

int 
sys_getparentid(void) 
{


  // struct proc *p;
  int pid;
  argint(0,&pid);
  cprintf("in syscall %d\n", pid);
  return getparent(pid);
}

int 
sys_getchildrenid(void) 
{
  char *children;
  int pid;
  argint(0, &pid);
  argptr(1, (void*)&children, sizeof(*children));
  return getchildren(pid, children);
}

struct rtcdate*
sys_gettime (void) 
{
  struct rtcdate* date;
  argptr(0, (void*)&date, sizeof(*date));
  cmostime(date);
  return date;
}

int
sys_recchildren (void) 
{
  char *children;
  int pid, last;
  argint(0, &pid);
  argptr(1, (void*)&children, sizeof(*children));
  last = getrecchildren(pid, children, 0);
  children[last] = '\0';
  return last;
}