BUILDDIR = ./.build
CFLAGS = -Wall 
CC = gcc

all: $(BUILDDIR)/deeprose

run:
	@make all
	rlwrap $(BUILDDIR)/deeprose

$(BUILDDIR)/%.o: %.c %.h
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c $^ 
	mv *.o $(BUILDDIR)/
	mv *.h.gch $(BUILDDIR)/

$(BUILDDIR)/deeprose: $(BUILDDIR)/lexer.o $(BUILDDIR)/arena.o $(BUILDDIR)/object.o main.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -r ./$(BUILDDIR)/
