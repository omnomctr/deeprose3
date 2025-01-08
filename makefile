BUILDDIR = ./.build
CFLAGS = -Wall -lgmp
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

$(BUILDDIR)/stdlib.h: programs/stdlib.deeprose
	python3 create_stdlib_header.py $(BUILDDIR)

$(BUILDDIR)/deeprose3: $(BUILDDIR)/lexer.o $(BUILDDIR)/arena.o $(BUILDDIR)/object.o $(BUILDDIR)/parser.o $(BUILDDIR)/eval.o $(BUILDDIR)/environment.o $(BUILDDIR)/stdlib.h $(BUILDDIR)/util.o main.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -r ./$(BUILDDIR)/
