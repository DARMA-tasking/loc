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
```

## Documentation

Make sure to have `doxygen` install. Tested with version `1.17.0`.

```bash
cd docs

# Generate
docs % doxygen Doxyfile

# Open documentation
docs % xdg-open html/index.html    # Linux
docs % open html/index.html        # macOS
```
