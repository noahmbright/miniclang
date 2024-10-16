### Miniclangpp

An eduacational C compiler targeting LLVM. Written in C++, using a couple of
it's niceties, i.e. a string and hash table implementation, along with enum
classes. Resources used in learning how to write a C compiler are included in
[references](#references), one of which is the LLVM Kaleidescope tutorial. 
The syntactic sugar and closeness to the tutorial are what motivated the 
choice, so alas, this compiler will not be able to compile itself. 

### Goals and non-goals

Fundamentally a learning project, the goal is not to make a perfect spec
compliant C compiler. A notable obstacle right now is the preprocessor, which
will get in the way of compiling realistic programs for the time being.
Learning LLVM is a higher priority, so there will be more focus on the internal
lower level details. Parsing C is enough of a front end challenge for now.

### References

The [C11 spec](https://www.open-std.org/jtc1/sc22/WG14/www/docs/n1570.pdf). The
canonical source of truth. 

Rui Ueyama's [chibicc](https://github.com/rui314/chibicc). Particularly useful
for the parser and seeing how to turn the spec's left recursive madness into
something reasonable.

Robert Nystrom's [Crafting
Interpreters](https://www.craftinginterpreters.com/). A good introduction and a
good resource for a second opinion. The intro knowledge is good for
demystifying and making compilers approachable, and it's goal is sufficiently
different from a C compiler that you need to make leaps between what you're
doing and what he shows, which is a good exercise. 
