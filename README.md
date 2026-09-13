# Salt

A compiled, statically typed programming language targeting native machine code via LLVM. Designed to do all the small things that make a project fit together

## Building

### Requirements
- CMake 4.3+
- Clang
- LLVM 22

### Build
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
```

## Usage

```bash
salt <file> [options]

Options:
  -o <name>      Output binary name (default: output)
  --emit-ir      Print LLVM IR to stdout
  --emit-tokens  Print token stream to stdout
  --version      Print version information
  --help         Print this help message
```

## Syntax Overview

### Annotations
```salt
@as { standalone };        // executable
@as { library };           // shared library
@requires { other };       // depend on other.salt
@meta { CURRENT_DIR, "../include" };  // build settings
```

### Functions
```salt
%i32 add(i32 a, i32 b) {
    return { a + b };
}
```

### Variables
```salt
%i32 x = 42;
%u8* msg = malloc(100);
%i32[10] arr;
```

### Control Flow
```salt
if (x > 0) {
    // ...
} else {
    // ...
}

while (x < 10) {
    if (x == 5) { break; }
    x = x + 1;
}
```

### Structs
```salt
%struct Point {
    %i32 .x;
    %i32 .y;
};

%Point p;
p.x = 5;
p.y = 10;

%Point* ptr = &p;
ptr->x = 20;
```

### External C Functions
```salt
@externC {
    void printf(i8* fmt, ...),
    void* malloc(i64 size)
};
```

## Type System

| Type | Description |
|------|-------------|
| `i8`/`u8` | 8-bit integer |
| `i16`/`u16` | 16-bit integer |
| `i32`/`u32` | 32-bit integer |
| `i64`/`u64` | 64-bit integer |
| `f32` | 32-bit float |
| `f64` | 64-bit float |
| `bool` | Boolean (u8) |
| `void` | No value |
| `T*` | Pointer to T |
| `T[N]` | Array of N T's |

## Known Limitations

- Linux only
- Debug builds only
- No standard library yet
- No garbage collection
- No generics
- No closures

## Example Program

```salt
@as { standalone };

@externC {
    void printf(i8* fmt, ...)
};

%i32 main(void) {
    printf("Hello, World!\n");
    return { SUCCESS };
}
```

## License

[LICENSE](LICENSE)
