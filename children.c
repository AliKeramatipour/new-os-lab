#include "types.h"
#include "stat.h"
#include "date.h"
#include "fcntl.h"
#include "user.h"

int main () {
    int pid;
    char children [128];
    pid = getpid();
    // printf(1, "%d\n", pid);
    // printf(1, "parent : %d\n" , getparentid(pid));
    if (fork() != 0 && fork() != 0 && fork() != 0) {
        getchildrenid(pid, children);
        printf(1, "CHILDREN %s\n", children);
        wait();
    }
    wait();
	exit(); 
}
