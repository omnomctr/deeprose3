shared.so: shared.o
	gcc shared.o -shared -o shared.so

shared.o: shared.c
	gcc -c shared.c -o shared.o

clean:
	rm shared.o shared.so

