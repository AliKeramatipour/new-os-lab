#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main (int argc, char** argv) {
  printf(1, "parent\n");
  print_process_table();
  int a, b;
  delay(1);
  if(fork() == 0){
    print_process_table();
    if((a = fork()) == 0){
      for(a = 0; a < 300 ; a++)
        printf(1, "+");
    }
    else{
      if((b = fork()) == 0){
        for(a = 0; a < 300 ; a++)
          printf(1, "*");
      }
      else{
        change_process_queue(getpid(), 2);
        change_process_queue(a, 2);
        change_process_queue(b, 2);
        for(a = 0; a < 300 ; a++)
          printf(1, "-");
        wait();  
        wait();
      }
    }
  }
  else{
    wait();
    printf(1, "\n");
  }
  exit();
}