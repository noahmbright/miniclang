# Miniclang

An educational C compiler targeting LLVM. Written in C++, using a couple of
it's niceties to allow for focus on compiler details. Miniclang will
unfortunately not be able to compile itself in the near future.

Codegen is done by emitting llvm as text. Not entirely ergonomic but that
decision was made to allow for learning LLVM and assembly-like programming
as opposed to learning an LLVM API.

LLVM IR is inherently SSA, so this allows for learning about that form
and the optimzations it enables. 

This readme is a living document essentially journalling what I've learned
throughout the process of writing this compiler.

# Stages of compilation

## Lexing

Lexing is done using a simple switch-driven lexer, very much inspired by 
Crafting Interpreters. This is definitely the least interesting part of the 
project.

## Parsing

The parser is split into three files, `parse_expressions.cpp` and
`parse_declarations.cpp`, and `parse_statements.cpp`. This is direct reflection
of the C specification, sections 6.5, 6.7, and 6.8. Expressions are where
operator precedences are defined, and is the easiest section to map to academic
resources on recursive descent parsing. This would be the best place to start
in implementing a parser on your own. Postfix operators also define things like
function calls, array indexing and struct member access.

Parsing statements is relatively simple given other parts of the parser - this
is where block statements (several statements wrapped in {}), and control flow
are defined. 

In addition to 6.8 on statements, `parse_statements.cpp` also handles section
6.9 on external definitions. This defines a translation unit, which is a `.c`
file that has been preprocessed. Unless you're writing a preprocessor, a
translation unit is just a file, as far as most people need to be concerned. 

### Parsing declarations

Parsing declarations is more complicated than parsing expressions. A
declaration is something like `int x, y[] = {1,2,}`. This involves sorting out
type information, propagating it forward appropriately, parsing initializers,
and figuring out how to represent this all in an AST.

### Scope

The parser initially checks for syntax errors only. It will accept undefined
variables or typedefs. These are checked in a subsequent semantic analysis
pass. This is more like clang than chibicc. Chibicc parses translation units
and loops over typedefs, global variable declarations and function definitions.
Here, we loop over global variables and function definitions only. See
[Actually parsing a file](#actually-parsing-a-file). 

This approach to variable references is reflected in how scopes are handled.
Scopes are linked lists, with their pointers their parent scopes. Scopes also
contain hashmaps mapping strings to `Object*`s, the string containing
identifier names. Declarations will populate a scope with identifiers and
typedef names. `ASTNodes` all contain pointers to these scopes. References to
these declarations are resolved when walking the AST.

Knowing that we are targeting LLVM IR informs decisions made in parsing, and 
what ends up going into the AST.

### Parsing Types

C is statically typed, meaning that our AST needs a way to represent data types
for use during static analysis. There are fundamental types such as `int` and 
`char`, but also pointer, function, array, and user defined types. Pointers point
to particular types, and pointers can point to pointers, so these are defined 
using linked lists to allow for sufficiently generic typing. A function pointer
type is defined by its return type and parameter list types, e.g. you can have
a pointer to a function that takes a `char` and returns an `int`.

### Actually parsing a file

Essentially the entry point to the compiler is the `parse_translation_unit`
function. This spins up a lexer and begins producing an AST. A translation unit
is a series of declarations or function definitions. Until you hit the `\0`
character, just keep chomping away. 

A C program is a series of declarations and function definitions, but this
requires disambiguation; `int x;` and `int x();` are both declarations, while
`int x(){...}` is a function definition. A function definition has the
parenthesis around its parameter list followed by the curly braces for the body
of the definition.

Most of the parsing functions return `ASTNode`s, which I think of as roots of
small abstract syntax trees (as opposed to parsing the program and making _the_
AST). Something like `int x, int y = 3 + 4 * 5;` is parsed as a declaration.
The returned `ASTNode` will represent the definition of `x`, and it will have a
pointer `next` pointing to the declaration of `y`, with `lhs` and `rhs`
recursively defining the expression `3 + 4 * 5`.

Because `next` in a declaration points to second identifier declared in a
single line, it doesn't entirely make sense to have `next` point to the next
declaration/function definition in a file. For this reason, the declarations in
a translation unit are wrapped in an `ExternalDelaration` struct. Name is
slightly funny but it's taken straight from 6.9 in the spec. This is a linked
list of stuff to make global objects/procedures for in the codegen stage.

## Codegen

(Much of this initial understanding comes from [Mapping High Level Constructs
to LLVM
IR](https://mapping-high-level-constructs-to-llvm-ir.readthedocs.io/en/latest/index.html)
)

### Variables

In LLVM, global variables and function names are prefixed with `@`. The entry
point into a program (i.e., the definition of the main function), will
typically look like `define i32 @main()`.

There are two types of local variables: temporaries and stack allocated. The
former are results of simply defining new symbols; the latter are results of
using the `alloca` instruction, which returns a pointer. Valid LLVM requires
that local variables use unique names to adhere to SSA form.

LLVM has the intrinsic `struct`, with fields mapping to 0-based indicies as
opposed to names. Indexing into structs and arrays involves use of the 
[`GetElementPtr` instruction](https://llvm.org/docs/GetElementPtr.html), deemed
confusing enough to get its own post on the LLVM website.

### Functions

Function definitions and declarations in LLVM resemble those in C. A definition
uses the `define` keyword, a comma-separated typed argument list, followed by
the body of the function wrapped in curly braces. A declaration uses the
`declare` keyword and a similar argument list - potentially with the variable
names omitted - e.g., `declare i32 @func(i32, i32)`.

### Branching and Phi functions

LLVM IR implements control flow by jumping between basic blocks, often ending
with `br` instructions, which can be either conditional or unconditional jumps.
When a new basic block begins, if it has several potential predeccsor blocks, a
`phi` function is used to collect the results of the predecessor that actually
executed and store them where they need to go. Any `phi` functions in a basic
block need to come before any non-`phi` functions.

# Status

Don't use this for anything. 

TODOs:

* Potentially refactor the lexer. Both to allow for preprocessing and to 
allow for testing the parser with independent token streams

* Decide on how/when to type check and type cast as we parse expressions

* Implement optimization passes 

# Goals and non-goals

Fundamentally a learning project, the goal is not to make a perfect spec
compliant C compiler. A notable obstacle right now is the preprocessor, which
will get in the way of compiling realistic programs for the time being.
Learning LLVM is a higher priority, so there will be more focus on the internal
lower level details. Parsing C is enough of a front end challenge for now.

To me, the most interesting part of this project is the middle end. How C-level
abstractions map to a lower-level IR, and how to perform optimizations on that
IR make for exciting questions both in a mathematical and implementational
sense.

A C-like coding style is used for the most part, up to and including and
unions. Following [chibicc](https://github.com/rui314/chibicc), there is not
yet any freeing of allocated resources, which is forgivable for a short-lived
program with high emphasis on speed.

It's a truism that global variables are bad practice, several of the resources
used here repeat that. In an attempt to take that to heart, globals are replaced
with pervasive pointer passing. As they are discovered, relative merits of this 
approach will be discussed here. Currently, this approach provides some
additional clarity in function calls and forces some additional intentionality
during programming, but it can also feel a bit repetitive. Benchmarking and comparing
the cost of passing around all the pointers would also be insightful.

# Building

Miniclang is a simple CMake project, so it should Just Work :tm: on most systems.

To clone and build, run the following:

```
git clone git@github.com:noahmbright/miniclang.git
cd miniclang
cmake -S . -B build
make -C build
```

# Testing

To run the (homegrown) test suite, run `./run_tests.sh` from the root of the
source tree. If no arguments are passed, all tests are run. By default, for
debug purposes, a CMake flag `TEST_VERBOSE` is set, which prints output to
`stdout` as the test cases are run. To test individual elements of the
compiler, the script can take a single command line argument. Currently
accepted arguments are `lexer`, `parser`.

`run_tests.sh` expects to find the test executables in a `build` directory. Please
adhere to the instructions in [building](#building) if you'd like the tests to 
just work.

# References

* The [C11 spec](https://www.open-std.org/jtc1/sc22/WG14/www/docs/n1570.pdf). The
canonical source of truth. 

* Rui Ueyama's [chibicc](https://github.com/rui314/chibicc). Particularly useful
for the parser and seeing how to turn the spec's left recursive madness into
something reasonable. Also very helpful in getting inspiration for representing
C's type system.

* Robert Nystrom's [Crafting
Interpreters](https://www.craftinginterpreters.com/). A good introduction and a
good resource for a second opinion. The intro knowledge is good for
demystifying and making compilers approachable, and it's goal is sufficiently
different from a C compiler that you need to make leaps between what you're
doing and what he shows, which is a good exercise. Credit for giving me the
courage to use unions. 

* [Mapping High Level Constructs to LLVM
  IR](https://mapping-high-level-constructs-to-llvm-ir.readthedocs.io/en/latest/index.html)

* [Matt Godbolt's Compiler Explorer](https://godbolt.org/). Turn on the `-emit-llvm` flag.
