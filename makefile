BUILDDIR = $(shell pwd)/.build
CFLAGS = -Wall -lgmp -O3 -ldl -fpic
CC = gcc

all: $(BUILDDIR)/deeprose3

install: $(BUILDDIR)/deeprose3
	cp $(BUILDDIR)/deeprose3 /usr/bin/

run:
	@make all
	rlwrap $(BUILDDIR)/deeprose3

$(BUILDDIR)/%.o: %.c %.h
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c $^ 
	mv *.o $(BUILDDIR)/
	mv *.h.gch $(BUILDDIR)/

$(BUILDDIR)/create_stdlib_header: create_stdlib_header.c
	@mkdir -p $(BUILDDIR)
	$(CC) create_stdlib_header.c -o $(BUILDDIR)/create_stdlib_header

$(BUILDDIR)/stdlib.h: programs/stdlib.deeprose $(BUILDDIR)/create_stdlib_header
	$(BUILDDIR)/create_stdlib_header <programs/stdlib.deeprose >$(BUILDDIR)/stdlib.h

$(BUILDDIR)/deeprose3: $(BUILDDIR)/lib/libdeeprose.so
	gcc -L$(BUILDDIR)/lib -o $(BUILDDIR)/deeprose3 main.c -ldeeprose -Wl,-rpath=$(BUILDDIR)/lib

$(BUILDDIR)/lib/libdeeprose.so: $(BUILDDIR)/lexer.o $(BUILDDIR)/arena.o $(BUILDDIR)/object.o $(BUILDDIR)/parser.o $(BUILDDIR)/eval.o $(BUILDDIR)/environment.o $(BUILDDIR)/stdlib.h $(BUILDDIR)/util.o
	@mkdir -p $(BUILDDIR)
	@mkdir -p $(BUILDDIR)/lib
	$(CC) $(CFLAGS) -shared -o $@ $^

clean:
	rm -r $(BUILDDIR)/
