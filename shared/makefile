shared.so: shared.o eval.o object.o
	gcc  shared.o eval.o object.o -shared -o shared.so

shared.o: shared.c
	gcc -c -fPIC shared.c -o shared.o

eval.o: ../eval.c
	gcc -c -fPIC ../eval.c -lgmp -ldl -o eval.o

object.o: ../object.c
	gcc -c -fPIC ../object.c -lgmp -ldl -o object.o
clean:
	rm shared.o shared.so eval.o object.o

