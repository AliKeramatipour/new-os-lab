#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

float stof(const char* s);
void ftoa(float n, char* res, int afterpoint) ;

int main (int argc, char** argv) {
  int priority1, priority2, priority3;
  priority1 = atoi(argv[1]);
  priority2 = atoi(argv[2]);
  priority3 = atoi(argv[3]);
  printf(1, "parent\n");
  print_process_table();
  int a, b;
  delay(1);
  if(fork() == 0){
    if((a = fork()) == 0){
      for(a = 0; a < 500 ; a++)
        printf(1, "+");
    }
    else{
      if((b = fork()) == 0){
        for(a = 0; a < 500 ; a++)
          printf(1, "*");
      }
      else{
        assign_srpf_priority(getpid(), priority1);
        assign_srpf_priority(a, priority2);
        assign_srpf_priority(b, priority3);
        change_process_queue(getpid(), 3);
        change_process_queue(a, 3);
        change_process_queue(b, 3);
        for(a = 0; a < 500 ; a++)
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




float stof(const char* s){
  float rez = 0, fact = 1;
  if (*s == '-'){
    s++;
    fact = -1;
  };
  for (int point_seen = 0; *s; s++){
    if (*s == '.'){
      point_seen = 1; 
      continue;
    };
    int d = *s - '0';
    if (d >= 0 && d <= 9){
      if (point_seen) fact /= 10.0f;
      rez = rez * 10.0f + (float)d;
    };
  };
  return rez * fact;
};
