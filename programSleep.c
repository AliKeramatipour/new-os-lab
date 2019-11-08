#include "types.h"
#include "stat.h"
#include "date.h"
#include "fcntl.h"
#include "user.h"

int main () {
    int difference;
    struct rtcdate time, time2;
    gettime(&time);
    printf(1, "Time 1: Second = %d Minute = %d Hour = %d Day = %d\n", time.second, time.minute, time.hour, time.day);
    delay(4);
    gettime(&time2);
    printf(1, "Time 2: Second = %d Minute = %d Hour = %d Day = %d\n", time2.second, time2.minute, time2.hour, time2.day);
    difference = ((time2.minute * 60) + time2.second) - ((time.minute * 60) + time.second);
    printf(1, "dif : %d\n", difference);
	exit();
}
