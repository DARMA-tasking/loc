## Upgrade external libraries procedure

### `fmt`

1. Download a new `fmt` release.
2. Replace `include/fmt/`.
3. Replace in `src/`:
    1. `src/fmt-c.cc`
    2. `src/format.cc`
    3. `src/os.cc`
4. Replace `ChangeLog.md`.
5. Replace `CMakeLists.txt`.
    * Comment out line 131: `include(JoinPaths)`.
6. Replace `README.md`.
7. Update `VERSION` (by hand).
