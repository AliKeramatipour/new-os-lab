#include "types.h"
#include "stat.h"
#include "date.h"
#include "fcntl.h"
#include "user.h"

int main () {
    int pid;
    pid = getpid();
    printf(1, "%d\n", pid);
    printf(1, "parent : %d\n" , getparentid(pid));
    if (fork() == 0) {
        pid = getpid();
        printf(1, "in if %d\n", pid);
        // getparentid(pid);
        printf(1, "child : %d\n" , getparentid(pid));

    }
    wait();
	exit(); 
}
