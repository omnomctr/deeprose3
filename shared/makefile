CFLAGS = -fPIC -lgmp

# link w/ libdeeprose found in $(BUILD)/lib/libdeeprose.so
shared.so: shared.o ../.build/lib/libdeeprose.so
	gcc $(CFLAGS) shared.o -L../.build/lib/ -Wl,-rpath=../.build/lib/ -ldeeprose -shared -o shared.so

shared.o: shared.c
	gcc -c $(CFLAGS) shared.c -o shared.o

clean:
	rm *.o shared.so
