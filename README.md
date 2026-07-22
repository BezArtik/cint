# cint -- C-like Interpreter

Tree-walk interpreter for a C-like language, written in C++20.


## Features

- Data types: int, double, bool, string, void
- Variables with lexical scoping
- Control flow: if/else, while, for
- Functions with parameters, return values, and type checking
- Built-in print function for console output
- Static type checking before execution
- Strict mode: type errors and undefined variables are caught before runtime


## Requirements

- CMake 3.20+
- C++20 compiler (GCC 11+, Clang 14+, MSVC 2022+)
- Ninja (for GCC presets) or Visual Studio


## Quick Start

### Build
```bash
git clone https://github.com/yourname/cint.git
cd cint
cmake --preset gcc-release
cmake --build --preset gcc-release
```
### Run
```bash
cint samples/fib.c
```

## Syntax

### Variables
```cpp
int x = 42;
double pi = 3.14;
bool flag = true;
string msg = "hello";
```
### Control Flow
```cpp
int x = -4;
if (x > 0) {
    print_str("positive");
} else {
    print_str("non-positive");
}
```
```cpp
int x = 10;
while (x > 0) {
    --x;
}
```
```cpp
for (int i = 0; i < 10; i++) {
    print_int(i);
}
```
### Functions
```cpp
int add(int a, int b) {
    return a + b;
}

void greet(string name) {
    print_str("Hello, ");
    print_str(name);
}

int result = add(3, 4);
print_int(result); // 7
greet("world");
```
### Structers
```cpp
struct Point {
    int x;
    int y;
};
struct Point p;
p.x = 10;
p.y = 15;
```

## Project Structure

- `src/`
    - `core/`          -- tokens, types, errors, utilities, values
    - `lexer/`         -- lexical analysis (source -> tokens)  
    - `ast/`           -- abstract syntax tree  
    - `parser/`        -- syntax analysis (tokens -> AST)  
    - `semantics/`     -- semantic analysis (type checking)  
    - `runtime/`       -- environment, execution  
    - `main.cpp`       -- entry point  
- `samples/`           -- example programs  


## Architecture

Pipeline: source -> Lexer -> tokens -> Parser -> AST -> TypeChecker -> checked AST -> Interpreter -> output

- Lexer -- finite automaton, splits characters into tokens
- Parser -- recursive descent (LL), builds AST
- TypeChecker -- walks AST, checks types and scopes
- Interpreter -- tree-walk AST execution


## License

MIT
