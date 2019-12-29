#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main () {
    int barrier_id = assign_barrier(3), barrier_id2 = assign_barrier(3), i, j;

    int parent = getpid();
    change_process_queue(parent, 2);
    for (i = 0 ; i < 5 ; i++) {
        if ((j = fork()) == 0) {
            break;
        }
        else {
            change_process_queue(j, 2);
        }
    }
    for ( j = 0 ; j < 10000 ; j++);
    if (i % 2) {
        printf(1, "%d arrived at %d\n", getpid(), barrier_id);
        arrive_at_barrier(barrier_id);
        printf(1, "%d passed the %d\n", getpid(), barrier_id);

    }
    else {
        printf(1, "%d arrived at %d\n", getpid(), barrier_id2);
        arrive_at_barrier(barrier_id2);
        printf(1, "%d passed the %d\n", getpid(), barrier_id2);
    }
    
    if (getpid() == parent) {
        for (i = 0 ; i  < 5 ; i++) {
            wait();
        }
        printf(1, "\n");
    }
    exit();
}