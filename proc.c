#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

const double MAX_PRIO = 1000;

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

struct barrier{
  int waiters;
  int channel;
  int arrived;
  int free;
};


struct barrier barriers[10];

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->queue = 2 ;
  p->executedCycles = 1 ;
  //MIGHT NEED TO CHECK LATER
  acquire(&tickslock);
  p -> arrivalTime = ticks;
  release(&tickslock);
  p->pid = nextpid++;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }
  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;
  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;
  np -> queue = 3 ;
  np -> priority = curproc -> priority + 100 ;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);
  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}
// Random Number generator for lottery scheduling
int randomNumberGenerator(int mx)
{
  int min = 0;
  uint rand = ticks * ticks; /* Any nonzero start state will work. */

    /*check for valid range.*/
    if(min == mx) {
        return min;
    }

    /*get the random in end-range.*/
    rand += 0x3AD;
    rand %= mx;

    /*get the random in start-range.*/
    while(rand < min){
        rand = rand + mx - min;
    }
  return rand;
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.

void ftoa(float n, char* res, int afterpoint);

void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    int sum = 0;
    int foundProcess = 0;

    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->queue != 1 )
        continue;
      if(p->state != RUNNABLE)
        continue;
      sum = sum + p -> lotteryChance ;
    }
    int randNumber = randomNumberGenerator(sum);
    sum = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->queue != 1 )
        continue;
      if(p->state != RUNNABLE)
        continue;
      if ( randNumber >= sum && randNumber < sum + p -> lotteryChance )
      {
        foundProcess = 1;
        p->executedCycles++;
        c->proc = p;
        switchuvm(p);
        p->state = RUNNING;
        swtch(&(c->scheduler), p->context);
        switchkvm();
        c->proc = 0;
        break;
      }
      sum = sum + p -> lotteryChance ;
    }

    //FOR second Queue
    if ( foundProcess == 0 ){
      struct proc *second_p;
      double maxHRRN = 0, HRRN = 0;
      for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->queue != 2 )
          continue;
        if(p->state != RUNNABLE)
          continue;
        acquire(&tickslock);
        HRRN = ticks - p->arrivalTime;
        release(&tickslock);
        HRRN = HRRN / ((double)p->executedCycles) ;
        if ( maxHRRN < HRRN ){
          maxHRRN = HRRN;
          second_p = p ;
          foundProcess = 1;
        }
      }
      if ( foundProcess == 1 ){
        p = second_p ;
        p -> executedCycles++ ;
        c->proc = p;
        switchuvm(p);
        p->state = RUNNING;
        swtch(&(c->scheduler), p->context);
        switchkvm();
        c->proc = 0;
      }
    }

    //THIRD QUEUE
    if ( foundProcess == 0 )
    {
      struct proc *third_p;
      double minPrio = MAX_PRIO ;
      // double maxPrio = 0 ;
      for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->queue != 3 )
          continue;
        if(p->state != RUNNABLE)
          continue;
        if ( minPrio > p -> priority )
        {
          minPrio = p -> priority ;
          third_p = p ;
          foundProcess = 1 ;
        }
        // if ( maxPrio < p -> priority )
        // {
        //   maxPrio = p -> priority ;
        //   third_p = p;
        //   foundProcess = 1 ;
        // }
      }
      if ( foundProcess == 1 ){
        p = third_p;
        if(p->priority > 0.1)
          p->priority -= 0.1;
        p->executedCycles++;
        c->proc = p;
        switchuvm(p);
        p->state = RUNNING;
        swtch(&(c->scheduler), p->context);
        switchkvm();
        c->proc = 0;
      }
    }    
    release(&ptable.lock);
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
   
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

int getparent (int pid) {
  struct proc *p;
  int parentPid = 0;
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      parentPid = p->parent->pid;
      release(&ptable.lock);
      return parentPid;
    }
  }
  release(&ptable.lock);
  return -1;
}

int pow(int a, int b){
  int i, res = 1;
  for(i = 0 ; i < b ; i++){
    res *= a;
  }
  return res;
}

int getchildren(int pid, char *children) {
  struct proc *p;
  int a = 0, max = 0, pos = 0, cpid;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent->pid == pid){
      cpid = p->pid;
      for(a = 0 ; cpid / pow(10, a) > 0 ; a++);
      max = a;
      for(; a > 0 ; a--){
        children[pos + a - 1] = cpid % 10 + 0x30;
        cpid /= 10;
      }
      pos += max;
    }
    if(pos >= 128){
      return -1;
    }
  }
  children[pos]= '\0';
  release(&ptable.lock);
  return 0;
}

int getrecchildren(int pid, char *children, int pos) {
  struct proc *p;
  int a = 0, max = 0, cpid;
  children[pos] = '\0';
  children[pos++] = '>';
  acquire(&ptable.lock);
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if (p->parent->pid == pid) {
      cpid = p->pid;
      for(a = 0 ; cpid / pow(10, a) > 0 ; a++);
      max = a;
      for(; a > 0 ; a--){
        children[pos + a - 1] = cpid % 10 + 0x30;
        cpid /= 10;
      }
      pos += max;
      release(&ptable.lock);
      pos = getrecchildren(p->pid, children, pos);
      acquire(&ptable.lock);
    }
    if (pos >= 128) {
      return -1;
    }
  }
  release(&ptable.lock);
  children[pos++] = '<';
  return pos;
}

int assignqueue(int pid, int queue) {
  struct proc *p;
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if(p->pid == pid){
      p->queue = queue;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

int assigntickets(int pid, int tickets) {
  struct proc *p;
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if(p->pid == pid){
      p->lotteryChance = tickets;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

int assignpriority(int pid, int priority) {
    struct proc *p;
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if(p->pid == pid){
      p->priority = priority;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

void reverse(char* str, int len) 
{ 
    int i = 0, j = len - 1, temp; 
    while (i < j) { 
        temp = str[i]; 
        str[i] = str[j]; 
        str[j] = temp; 
        i++; 
        j--; 
    } 
} 
  
int intToStr(int x, char str[], int d) 
{ 
    int i = 0; 
    while (x) { 
        str[i++] = (x % 10) + '0'; 
        x = x / 10; 
    } 
  
    while (i < d) 
        str[i++] = '0'; 
  
    reverse(str, i); 
    str[i] = '\0'; 
    return i; 
} 
  
void ftoa(float n, char* res, int afterpoint) 
{ 
    int ipart = (int)n; 
  
    float fpart = n - (float)ipart; 
  
    int i = intToStr(ipart, res, 0); 
  
    if (afterpoint != 0) { 
        res[i] = '.'; 

        fpart = fpart * pow(10, afterpoint); 
  
        intToStr((int)fpart, res + i + 1, afterpoint); 
    } 
} 

void printproctable() {
  struct proc *p;
  static char *states[] = {
  [UNUSED]    "UNUSED",
  [EMBRYO]    "EMBYRO",
  [SLEEPING]  "SLEEPING",
  [RUNNABLE]  "RUNNABLE",
  [RUNNING]   "RUNNING",
  [ZOMBIE]    "ZOMBIE"
  };
  char prio[20], HRRNstr[20];
  int afterdot = 3;
  float HRRN;
  uint now, wait;
  acquire(&ptable.lock);
  cprintf("name,    pid,    state,    queue number,    priority,    lottery chance,    num of cycles,    HRRN\n");
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if (p->pid != 0) {
      acquire(&tickslock);
      now = ticks;
      release(&tickslock);
      wait = now - p->arrivalTime;
      HRRN = wait/p->executedCycles;
      ftoa(p->priority, prio, afterdot);
      ftoa(HRRN, HRRNstr, afterdot);
      cprintf("%s,    %d,    %s,    %d,    %s,    %d,    %d,    %s\n",
        p->name, p->pid, states[p->state], p->queue, prio, p->lotteryChance, p->executedCycles, HRRNstr
      );
    }
  }
  release(&ptable.lock);
  return;
}


int assign_barrier(int number){
  int i = 0, index = -1;
  for(i = 0 ; i < 10 ; i++){
    if(barriers[i].free == 0){
      index = i;
      break;
    }
  }
  if(index == -1)
    return -1;
  barriers[index].waiters = number;
  barriers[index].free = 1;
  barriers[index].arrived = 0;
  return index;
}

int arrive_at_barrier(int index){

  if(barriers[index].free == 0 || index >= 10)
    return -1;
  if(barriers[index].arrived + 1 == barriers[index].waiters){
    wakeup(&barriers[index].channel);
    barriers[index].free = 0;
    return 0;
  }
  barriers[index].arrived++;
  acquire(&ptable.lock);
  sleep(&barriers[index].channel, &ptable.lock);
  release(&ptable.lock);
  return 1;
}

void reqursive_test(int pid, int depth){
  if(depth == 0)
    return;
  acquire_rn(&ptable.lock, pid);
  cprintf("acquired\n");
  reqursive_test(pid, depth - 1);
}

int test_reentrant_spinlock(int pid){
  acquire_rn(&ptable.lock, pid);
  cprintf("acquired\n");
  reqursive_test(pid, 10);
  release(&ptable.lock);
  cprintf("released\n");
  return 0;
}

