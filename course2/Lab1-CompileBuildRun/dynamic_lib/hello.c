#include <stdio.h>
#include <unistd.h>
void hello_from_dynamic_lib();

int main(){
	sleep(60);
	hello_from_dynamic_lib();
	return 0;
}
