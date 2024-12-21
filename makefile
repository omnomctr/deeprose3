BUILDDIR = ./.build
CFLAGS = -Wall -ggdb
CC = gcc

all: $(BUILDDIR)/deeprose

run:
	@make all
	rlwrap $(BUILDDIR)/deeprose -repl

$(BUILDDIR)/%.o: %.c %.h
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c $^ 
	mv *.o $(BUILDDIR)/
	mv *.h.gch $(BUILDDIR)/

$(BUILDDIR)/stdlib.h: programs/stdlib.deeprose
	python3 create_stdlib_header.py $(BUILDDIR)

$(BUILDDIR)/deeprose: $(BUILDDIR)/lexer.o $(BUILDDIR)/arena.o $(BUILDDIR)/object.o $(BUILDDIR)/parser.o $(BUILDDIR)/eval.o $(BUILDDIR)/environment.o $(BUILDDIR)/stdlib.h main.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -r ./$(BUILDDIR)/
