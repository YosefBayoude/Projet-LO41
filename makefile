HEADERS = projet.h

default: projet

projet.o: projet.c $(HEADERS)
	gcc -c projet.c -o projet.o

projet: projet.o
	gcc projet.o -o projet
	-rm -f projet.o
	./projet

clean:
	-rm -f projet.o
	-rm -f projet