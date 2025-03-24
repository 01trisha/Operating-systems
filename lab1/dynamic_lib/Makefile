compile:
	cd lib/ &&	gcc -shared -fPIC -o libdynamic.so libdynamic.c
	cd ..
	gcc -o hello hello.c -L./lib/ -ldynamic -Wl,-rpath,/home/parallels/Desktop/lab1/dynamic_lib/lib/

clean:
	cd lib/ && rm *.so
	cd ..
