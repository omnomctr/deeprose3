CFLAGS = -fPIC -lgmp

shared.so: shared.o eval.o object.o environment.o parser.o arena.o util.o lexer.o
	gcc $(CFLAGS) shared.o eval.o object.o environment.o parser.o arena.o util.o lexer.o -shared -o shared.so

shared.o: shared.c
	gcc -c $(CFLAGS) shared.c -o shared.o

%.o: ../%.c
	gcc $(CFLAGS) -c $^

clean:
	rm *.o shared.so
