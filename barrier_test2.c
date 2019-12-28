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
    for ( j = 0 ; j < 100 ; j++)
        printf(1, "*");
    if (i % 2) {
        arrive_at_barrier(barrier_id);
        while(check_barrier(barrier_id) == 0);
        printf(1, "###");
    }
    else {
        arrive_at_barrier(barrier_id2);
        while(check_barrier(barrier_id2) == 0);
        printf(1, "&&&");
    }
    
    if (getpid() == parent) {
        for (i = 0 ; i  < 5 ; i++) {
            wait();
        }
        printf(1, "\n");
    }
    exit();
}