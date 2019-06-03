#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(){
  int f = fork();

  if(f == 0){
    f = fork();
    if(f > 0){
      printf("1. proses id (1. seviye) : %d\n", getppid() );
      printf("1. prosesin çocuğu 2. proses id (2. seviye): %d\n", getpid() );
      printf("2. prosesin çocuğu 4. proses id (3. seviye): %d\n", f );
    }
  }
  else{
    f = fork();
    if(f == 0){
      printf("1. prosesin çocuğu 3. proses id (2. seviye): %d\n", getpid());
      f = fork();
      if(f == 0){
        printf("3. prosesin çocuğu 5. proses id (3. seviye): %d\n" , getpid());
      }
      else{
        f = fork();

        if(f == 0){
          printf("3. prosesin çocuğu 6. proses id (3. seviye): %d\n", getpid());
          f = fork();

          if(f == 0){
            printf("6. prosesin çocuğu 7. proses id (4. seviye): %d\n", getpid());
          }
          else{
            f = fork();

            if(f == 0){
              printf("6. prosesin çocuğu 8. proses id (4. seviye): %d\n", getpid());
            }
          }
        }
      }
    }
  }
}
