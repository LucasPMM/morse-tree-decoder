# Morse Tree Decoder

A C17 command-line decoder using a binary trie built from a text mapping. Dots
select left children, dashes select right children, and `/` separates words.
Created by Lucas Paulo Martins Mariz as a 2019 data structures assignment.

This modernization is currently at roadmap phases 1-3: project organization,
module extraction, and regression coverage. The historical table's nine incorrect
letter mappings and legacy output formatting are intentionally retained until
phase 4. See the [decoding contract](docs/decoding-contract.md) for current versus
planned behavior. This is not yet a standards-correct Morse decoder.

## Build and run

Requirements: a C17 compiler (GCC or Clang) and GNU Make. Tests also need a POSIX
shell, `timeout`, GNU-compatible `head`, and standard command-line tools.

Run from the repository root so the default `data/morse.txt` path resolves:

```sh
make
./morse < examples/sample.in
./morse -a < examples/sample.in
```

The sample decodes to `SOS HELP`; the current output has no final newline. `-a`
appends the mapped tree in preorder. It currently joins its first record to the
last message; that historical defect is covered by a temporary regression fixture.

## Development

| Command | Purpose |
| --- | --- |
| `make test` | Direct C module tests and exact-byte CLI baseline comparisons |
| `make sanitize` | The same suite with AddressSanitizer and UBSan |
| `make coverage` | Run instrumented tests and print gcov coverage |
| `make analyze` | Compile with the GCC static analyzer |
| `make format` | Format C source/headers with clang-format |
| `make check-format` | Check formatting without modifying files |
| `make clean` | Remove only known generated profile directories and executable |

Release, test, sanitizer, coverage, and analyzer objects are isolated under
`build/`. Compiler/flag changes invalidate the corresponding profile. Use
`make CC=clang test` to select Clang and `make CLANG_FORMAT=clang-format-18 check-format`
to select a versioned formatter. Coverage requires GCC/gcov; static analysis
requires GCC; formatting requires clang-format.

Local leak detection defaults to disabled for traced/sandboxed environments.
On a compatible host, use `make sanitize ASAN_OPTIONS=detect_leaks=1`.

`src/` contains the application, tree, table loader, and decoder modules;
`include/` contains their interfaces; `data/` contains the active table;
`examples/` contains runnable input; and `tests/` contains direct tests and
historical/CLI fixtures.

The [original report](docs/original-report-2019.pdf) and
[extended historical tree diagram](docs/figures/morse-tree.png) are preserved
unchanged. The chart shows additional symbols outside the application's scope.
The complete English results/evidence summary and CI workflow remain scheduled
for later roadmap phases; no hosted CI result is claimed at this stage.
