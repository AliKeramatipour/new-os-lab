#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main () {
    int barrier_id = assign_barrier(6), i, j;
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
    for ( j = 0 ; j < 100000 ; j++);
    printf(1, "proccess %d arrived at barrier\n", getpid());

    arrive_at_barrier(barrier_id);
    printf(1, "proccess %d passed the barrier\n", getpid());

    if (getpid() == parent) {
        for (i = 0 ; i  < 5 ; i++) {
            wait();
        }
        printf(1, "\n");
    }
    exit();
}