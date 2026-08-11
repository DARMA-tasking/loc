# loc => location coordinator

Distributed resolver for migratable entities with an injectable communication
backend. See [docs/md/overview.md](docs/md/overview.md) and
[docs/md/architecture.md](docs/md/architecture.md).

## Standalone build

Top-level builds enable the standalone `comm` backend and direct MPI support by
default. `comm` brings magistrate/checkpoint and fmt transitively. Build and
install `comm` first, then point `loc` at it with
`-Dcomm_DIR=<comm-install>/cmake`.

## Read the documentation

To learn *loc*, read the
[full documentation](https://darma-tasking.github.io/loc_docs/html/index.html)
that is automatically generated whenever a push occurs to "master".

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

## Embedding as a TPL

When another project adds `loc` as a subdirectory, standalone dependencies,
examples, tests, and documentation default to disabled:

```cmake
add_subdirectory(path/to/loc)
target_link_libraries(my_target PRIVATE loc::loc)
```

The embedding project supplies a communication type satisfying
`loc::Communicator`; neither standalone `comm` nor MPI is found or linked by
`loc`. The defaults can also be selected explicitly:

```bash
cmake -S . -B build \
  -DLOC_ENABLE_COMM=OFF \
  -DLOC_ENABLE_MPI=OFF \
  -DLOC_BUILD_EXAMPLES=OFF \
  -DLOC_BUILD_TESTS=OFF \
  -DLOC_BUILD_DOCS=OFF
```

Use `tests/tpl` as a dependency-free consumer smoke test:

```bash
cmake -S tests/tpl -B build/tpl
cmake --build build/tpl
ctest --test-dir build/tpl --output-on-failure
```

## Upgrade external libraries procedure

* See [lib/UPDATE.md](lib/UPDATE.md)
