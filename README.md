# loc => location coordinator

Distributed resolver for migratable entities, built on the standalone `comm`
communication library. See [docs/md/overview.md](docs/md/overview.md) and
[docs/md/architecture.md](docs/md/architecture.md).

## Required

`loc` links an installed `comm` via `find_package(comm CONFIG)`, which brings MPI,
magistrate/checkpoint, and fmt transitively. Build and install `comm` first, then
point `loc` at it with `-Dcomm_DIR=<comm-install>/cmake`.

## Included workflows

* See [.github/workflows/README](.github/workflows/README.md)

## Usage

With scripts:
```bash
# Building
## If /comm is next to /loc
./ci/build_cpp.sh "$PWD" "$PWD/build/ci"
## Else
./ci/build_cpp.sh "$PWD" "$PWD/build/ci" /path/to/comm/install/cmake

# Testing
./ci/test_cpp.sh "$PWD" "$PWD/build/ci"
```

With cmake:
```bash
# Building (point at an installed comm)
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/comm/install/cmake

# Compiling
cmake --build build --parallel

# Testing
ctest --test-dir build --output-on-failure
mpirun -np 2 ./build/tests/loc_tests   # cross-rank resolver protocol

# Examples
mpirun -np 2 ./build/examples/resolve

# Documentation
open build/html/index.html # macOS (xdg-open on Linux)
```

## Upgrade external libraries procedure

* See [lib/UPDATE.md](lib/UPDATE.md)
