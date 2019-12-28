#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main () {
    test_reentrant_spinlock(getpid());
    printf(1, "success\n");
    exit();
}