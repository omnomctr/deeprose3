# deeprose3

lisp-based language. Look at the example programs and standard library in [programs/](https://github.com/omnomctr/deeprose3/tree/main/programs).
You can see builtin functions / macros in eval.c's builtins[].

has a garbage collector, lists as first class functions and extensibility with the c programming language

# install

**dependencies**: gcc, make, rlwrap

``
$ git clone https://github.com/omnomctr/deeprose3

$ cd deeprose3
$ make # to compile, find the binary in .build/
$ make run # compile and run rlwrap deeprose3 
``

