# deeprose3

lisp-based language. Look at the example programs in [programs/](https://github.com/omnomctr/deeprose3/tree/main/programs).
You can see all builtin functions / special forms in eval.c's builtins[].

has a garbage collector, lists as first class citizens, higher order functions and extensibility with the C programming language

```lisp
(def quicksort (\ (xs)
    (and xs
        (let (x (first xs)
              xs (rest xs))
          (concat (quicksort (filter (\ (n) (< n x)) xs))
                  (list x)
                  (quicksort (filter (\ (n) (>= n x)) xs)))))))

```

# install

**dependencies**: [GCC](https://gcc.gnu.org/), [make](https://www.gnu.org/software/make/), [GMP](https://gmplib.org/), [readline](https://www.gnu.org/software/readline/) (should all be automatically installed on a GNU/Linux system)

```sh
$ git clone https://github.com/omnomctr/deeprose3

$ cd deeprose3
$ make # to compile, find the binary in .build/
$ make run # compile and run deeprose3 
$ sudo make install # install to /usr/bin
```

# TODO
find all TODO's in the code:
```ch
grep -rn "TODO" *.[ch]
```
* add dynamic loading with `dlopen`? **DONE**
* add namespaces maybe
* gut and replace parser with something less awfull (maybe yacc?)
