# loc => location manager

## Included workflows

* See [.github/workflows/README](.github/workflows/README.md)

## Usage

```bash
# Building
cmake -S . -B build

# Compiling
cmake --build build

# Testing
./build/tests/loc_tests

# Examples
./build/examples/[filename]
# example: ./build/examples/dummy1

# Documentation
xdg-open build/html/index.html # Linux
open build/html/index.html # macOS
```
