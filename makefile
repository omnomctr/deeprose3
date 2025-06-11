BUILDDIR = $(shell pwd)/.build
CFLAGS = -Wall -O3
SHAREDCFLAGS = $(CFLAGS) -lgmp -ldl -fpic
CC = gcc

all: $(BUILDDIR)/deeprose3

install: $(BUILDDIR)/deeprose3
	cp $(BUILDDIR)/deeprose3 /usr/bin/

run:
	@make all
	$(BUILDDIR)/deeprose3

$(BUILDDIR)/%.o: %.c %.h
	@mkdir -p $(BUILDDIR)
	$(CC) $(SHAREDCFLAGS) -c $^ 
	mv *.o $(BUILDDIR)/
	mv *.h.gch $(BUILDDIR)/

$(BUILDDIR)/create_stdlib_header: create_stdlib_header.c
	@mkdir -p $(BUILDDIR)
	$(CC) create_stdlib_header.c -o $(BUILDDIR)/create_stdlib_header

$(BUILDDIR)/stdlib.h: stdlib.deeprose $(BUILDDIR)/create_stdlib_header
	$(BUILDDIR)/create_stdlib_header <stdlib.deeprose >$(BUILDDIR)/stdlib.h

$(BUILDDIR)/deeprose3: $(BUILDDIR)/lib/libdeeprose.so main.c
	gcc -L$(BUILDDIR)/lib -o $(BUILDDIR)/deeprose3 main.c -ldeeprose -lreadline -Wl,-rpath=$(BUILDDIR)/lib $(CFLAGS)

$(BUILDDIR)/lib/libdeeprose.so: $(BUILDDIR)/lexer.o $(BUILDDIR)/arena.o $(BUILDDIR)/object.o $(BUILDDIR)/parser.o $(BUILDDIR)/eval.o $(BUILDDIR)/environment.o $(BUILDDIR)/stdlib.h $(BUILDDIR)/util.o
	@mkdir -p $(BUILDDIR)
	@mkdir -p $(BUILDDIR)/lib
	$(CC) $(SHAREDCFLAGS) -shared -o $@ $^

clean:
	rm -r $(BUILDDIR)/
