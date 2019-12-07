#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main (int argc, char** argv) {
  int lottory1, lottory2;
  lottory1 = atoi(argv[1]);
  lottory2 = atoi(argv[2]);
  printf(1, "parent\n");
  print_process_table();
  int a;
  delay(2);
  if(fork() == 0){
    print_process_table();
    if((a = fork()) == 0){
      for(a = 0; a < 500 ; a++)
        printf(1, "+");
      }
    else{
      assign_tickets(a, lottory2);
      assign_tickets(getpid(), lottory1);
      change_process_queue(getpid(), 1);
      change_process_queue(a, 1);
      for(a = 0; a < 500 ; a++)
        printf(1, "-");
      wait();  
    }
  }
  else{
    wait();
    printf(1, "\n");
  }
	exit();
}