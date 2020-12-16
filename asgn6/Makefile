all: mush

mush: mush.o functions.o
	gcc -Wall -pedantic -g -o mush mush.o functions.o

mush.o: mush.c functions.h
	gcc -Wall -pedantic -g -c mush.c

functions.o: functions.c functions.h
	gcc -Wall -pedantic -g -c functions.c

clean:
	rm mush mush.o functions.o