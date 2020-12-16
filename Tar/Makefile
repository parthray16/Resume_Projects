all: mytar

mytar: mytar.o tar.o safe_mem_functions.o
	gcc -Wall -pedantic -g -o mytar mytar.o tar.o safe_mem_functions.o

mytar.o: mytar.c tar.h
	gcc -Wall -pedantic -g -c mytar.c

tar.o: tar.c tar.h
	gcc -Wall -pedantic -g -c tar.c

safe_mem_functions.o: safe_mem_functions.c safe_mem_functions.h
	gcc -Wall -pedantic -g -c safe_mem_functions.c

clean:
	rm mytar safe_mem_functions.o tar.o mytar.o