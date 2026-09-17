# Morse Tree Decoder

A C17 command-line decoder using a binary trie built from a text mapping. Dots
select left children, dashes select right children, and `/` separates words.
Created by Lucas Paulo Martins Mariz as a 2019 data structures assignment.

Roadmap phases 1-6 now cover project organization, module extraction, corrected
international alphanumeric mappings, validated input, comprehensive tests, and CI.
See the [decoding contract](docs/decoding-contract.md) for behavior, limits, and
compatibility changes. The full historical results summary remains phase 7.

## Build and run

Requirements: a C17 compiler (GCC or Clang) and GNU Make. Tests also need a glibc/POSIX
host, a GNU-compatible linker, a POSIX shell, `awk`, `timeout`, GNU-compatible `head`,
and standard command-line tools. The application itself has no third-party dependencies.

Run from the repository root so the default `data/morse.txt` path resolves:

```sh
make
./morse < examples/sample.in
./morse -a < examples/sample.in
./morse --print-tree < examples/sample.in
./morse --help
```

The sample prints `SOS HELP` followed by a newline. `-a` or `--print-tree` appends
the mapped tree in preorder, with each record on a separate line. Spaces and tabs
separate codes; a standalone `/` emits a word separator. LF, CRLF, and EOF without
a final newline are accepted, including dynamically read long lines.

Invalid input or arguments produce an English diagnostic on standard error and a
nonzero exit status. An invalid line is omitted, although earlier valid lines may
already have been printed. The bundled table supports uppercase `A-Z` and `0-9` only.

## Development

| Command | Purpose |
| --- | --- |
| `make test` | Module, exhaustive, fault-injection, and exact-byte CLI tests |
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

The [CI workflow](.github/workflows/ci.yml) runs GCC and Clang tests, sanitizers with
leak detection, formatting, static analysis, and coverage. Every job and step has
an explicit timeout. Hosted CI can only be confirmed after an approved push.

The [original report](docs/original-report-2019.pdf) and
[extended historical tree diagram](docs/figures/morse-tree.png) are preserved
unchanged. The chart shows additional symbols outside the application's scope.
The complete English results/evidence summary remains scheduled for phase 7;
no hosted CI result is claimed at this stage.
