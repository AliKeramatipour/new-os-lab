#include "types.h"
#include "stat.h"
#include "date.h"
#include "fcntl.h"
#include "user.h"


int main (int argc, char** argv) {
    int i = 0 ;
    int curInp, prevRegValue;
    asm ("movl %%esi, %0" : "=r" (prevRegValue));
    for (i = 1 ; i < argc ; i++) {
        curInp = atoi(argv[i]);
        asm ("movl %0, %%esi;"::"r"(curInp) :"%esi");    
        printf(1, "%d has %d digits\n",curInp, count_num_of_digits());
    }
    asm volatile ("movl %0, %%esi"::"r"(prevRegValue):"%esi" );
	exit(); 
}
