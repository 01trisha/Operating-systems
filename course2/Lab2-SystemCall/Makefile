hello:
	gcc hello.c -o hello
	./hello
	strace ./hello
write:
	gcc write_hello.c -o write_hello
	./write_hello
	strace ./write_hello
syscall:
	gcc syscall_hello.c -o syscall_hello
	./syscall_hello
	strace ./syscall_hello
wget:
	strace wget kernel.org
	strace -c wget kernel.org
without_syscall:
	gcc without_syscall_hello.c -o without_syscall_hello
	./without_syscall_hello
	strace ./without_syscall_hello

clear:
	rm hello index.* syscall_hello write_hello without_syscall_hello 
