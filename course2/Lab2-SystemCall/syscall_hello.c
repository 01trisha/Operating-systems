#include <unistd.h>
#include <sys/syscall.h>


int main(){
  //syscall(SYS_write, 1, "Hello World!", 12);
  write(1, "Hello World!", 12);

  return 0;
}
