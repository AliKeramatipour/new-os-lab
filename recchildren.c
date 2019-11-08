#include "types.h"
#include "stat.h"
#include "date.h"
#include "fcntl.h"
#include "user.h"

int main () {
    int pid;
    char children [128];
    pid = getpid();
    fork();
    wait();
    if(fork() != 0){
        recchildren(pid, children);
        printf(1, "CHILDREN %s\n", children);
        wait();
    }
    wait();
	exit(); 
}
