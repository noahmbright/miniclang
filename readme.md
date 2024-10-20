#Miniclangpp

An eduacational C compiler targeting LLVM. Written in C++, using a couple of
it's niceties, i.e. a string and hash table implementation, along with enum
classes. Resources used in learning how to write a C compiler are included in
[references](#references), one of which is the LLVM Kaleidescope tutorial. 
The syntactic sugar and closeness to the tutorial are what motivated the 
choice, so alas, this compiler will not be able to compile itself. 

## Goals and non-goals

Fundamentally a learning project, the goal is not to make a perfect spec
compliant C compiler. A notable obstacle right now is the preprocessor, which
will get in the way of compiling realistic programs for the time being.
Learning LLVM is a higher priority, so there will be more focus on the internal
lower level details. Parsing C is enough of a front end challenge for now.

C's dangerous features are used, i.e., raw pointers allocated via `malloc` and 
unions. Following [chibicc](https://github.com/rui314/chibicc), there is not yet
any freeing of allocated resources, which is forgivable for a short-lived program
with high emphasis on speed.

Testing infrastructure is homegrown. 

It's a truism that global variables are bad practice, several of the resources
used here repeat that. In an attempt to take that to heart, globals are replaced
with pervasive pointer passing. As they are discovered, relative merits of this 
approach will be discussed here. Currently, this approach provides some
additional clarity in function calls and forces some additional intentionality
during programming, but it can also feel a bit repetitive. Benchmarking and comparing
the cost of passing around all the pointers would also be insightful.

## Building

Miniclangpp is a simple CMake project, so it should Just Work:tm: on most systems.

To clone and build, run the following:

```
git clone git@github.com:noahmbright/miniclangpp.git
cd miniclangpp
cmake -S . -B build
make -C build
```

## Testing

To run the (homegrown) test suite, run `./run_tests.sh` from the root of the
source tree. If no arguments are passed, all tests are run. By default, for debug
purposes, a CMake flag `TEST_VERBOSE` is set, which prints output to `stdout` as
the test cases are run. To test individual elements of the compiler, the script
can take a single command line argument. Currently accepted arguments are `lexer`.

`run_tests.sh` expects to find the test executables in a `build` directory. Please
adhere to the instructions in [building](#building) if you'd like the tests to 
just work.

## References

The [C11 spec](https://www.open-std.org/jtc1/sc22/WG14/www/docs/n1570.pdf). The
canonical source of truth. 

Rui Ueyama's [chibicc](https://github.com/rui314/chibicc). Particularly useful
for the parser and seeing how to turn the spec's left recursive madness into
something reasonable. Also very helpful in getting inspiration for representing
C's type system.

Robert Nystrom's [Crafting
Interpreters](https://www.craftinginterpreters.com/). A good introduction and a
good resource for a second opinion. The intro knowledge is good for
demystifying and making compilers approachable, and it's goal is sufficiently
different from a C compiler that you need to make leaps between what you're
doing and what he shows, which is a good exercise. Credit for giving me the
courage to use unions.
