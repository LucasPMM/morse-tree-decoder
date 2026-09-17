# Decoding Contract

Morse Tree Decoder (`morse-tree-decoder`) is a C command-line decoder based on a
file-backed binary trie. A dot selects the left child and a dash selects the
right child. The root and intermediate nodes may be empty; only mapped nodes
represent decoded characters. Roadmap phases 4-6 implement the contract below.

## Build and command line

Build from the repository root with `make`, then execute:

```sh
./morse < examples/sample.in
./morse -a < examples/sample.in
./morse --print-tree < examples/sample.in
./morse --help
```

The executable name remains `morse`; the selected repository name does not rename
the binary or change the remote yet. Accept no arguments or exactly one of
`-a`, `--print-tree`, and `--help`. Unknown options, filenames, repeated options,
and extra arguments fail with an English diagnostic and a nonzero exit status.

Help is printed on standard output and does not read input or open the table.
Normal decoding loads `data/morse.txt` relative to the working directory, so the
documented commands run from the repository root. There is no automatic search
beside the executable or `--table` option. The table module accepts an explicit
path for direct use/tests, and compilation can override `MORSE_TABLE_PATH` through
`CPPFLAGS`. The default is not an absolute build-machine path.

## Mapping format

The bundled table contains international Morse mappings for uppercase `A-Z` and
digits `0-9`. This project does not add lowercase, punctuation, accented letters,
an encoder, audio, or a legacy-mode option.

Each nonblank record contains exactly one supported symbol, whitespace, and one
code of 1-5 dot/dash signals:

```text
E .
T -
A .-
```

Spaces/tabs may surround fields and separate them. Blank lines, LF, CRLF, and EOF
without a final newline are accepted. Internal/bare CR, comments, extra fields,
invalid symbols/signals, oversized codes, NUL, non-ASCII, and unsupported controls
are rejected. Records are read as complete dynamically buffered lines, not fixed chunks.

A custom table may contain a subset of the supported alphabet, but must be nonempty.
Both symbols and codes must be unique. The loader checks uniqueness before insertion;
the low-level trie operation still replaces the symbol when reinserting a code,
preserving that original data-structure operation independently of the file contract.

## Message format

Each physical input line is a separate message. Spaces and tabs delimit tokens.
A standalone `/` emits exactly one space, including leading, trailing, and
consecutive `/` tokens. Attached forms such as `.../---` are invalid.

Accept LF, CRLF, and an unterminated final line. Blank lines and whitespace-only
lines decode to empty lines. Empty input produces no decoded output. Codes not
present in the loaded table, invalid signals, oversized tokens, NUL, non-ASCII,
DEL, and unsupported controls fail rather than becoming guessed letters or dashes.
An internal or unterminated CR is invalid; only a CR immediately before LF is removed.

The input buffer grows geometrically with checked arithmetic. Objects are limited
to `PTRDIFF_MAX` bytes and available memory; there is no fixed 500-byte line limit.
The application reserves sufficient decoded space before processing a line, and
writes it only after complete successful decoding.

## Output, ownership, and failure handling

Every successful decoded line ends with exactly one LF, including empty lines.
Output is streamed: if a later line is invalid, prior valid lines may already be
printed, but the invalid line and the optional tree are omitted. Diagnostics use
standard error, identify the line/token where applicable, and return nonzero.

`-a` and `--print-tree` print all mapped nodes after successful decoding, one
`symbol code` record per line, in preorder (root, left, right). Empty/root nodes
are omitted. The tree is never concatenated to the last message. With empty input,
the tree may be printed alone.

Allocation, table opening/parsing, stream reading/writing/flushing, and table-close
failures are checked. I/O errors can leave already emitted output; line-level
validation is not an all-or-nothing transaction for the whole file. All partial
trees and buffers are destroyed on failure. Application streams are borrowed
and remain the caller's responsibility; the table loader closes its own file.

The decoder borrows an immutable newline-free string and caller-owned output buffer,
does no stream I/O/allocation, and returns a status plus an optional one-based failing
token. Its output is only usable on success. Tree lookup entries are borrowed until
modification/destruction. `TextBuffer` values start with `{0}`, own their storage,
and remain destroyable after any failure.

## Historical evidence and deliberate corrections

The [unchanged 2019 report](original-report-2019.pdf) describes trie construction,
lookup, a literal input example, preorder printing, and theoretical complexity.
It contains no enumerated test suite, timing measurements, or expected output for
that example. The archived PDF is preserved in its original language.

The [archived diagram](figures/morse-tree.png) was already tracked in the original
repository and appears in the PDF. Its external origin/license is not stated
there; no new attribution or license is invented. It shows punctuation, accented
letters, and multi-character symbols outside this application's supported alphabet,
and is not an exact rendering of the original buggy table.

The literal report input remains unchanged in
`tests/fixtures/historical/report-example.in`:

```text
--- ... / -. . - --- ... / --.- .. .
```

The original baseline compiled from commit `96d7bf0` emitted `OS MEDOS QUE` without
a final LF during this review. The corrected implementation emits `OS NETOS QIE`
with a final LF, verified by an active CLI test. Neither output was recorded in
the PDF. Do not edit the source input to silently turn `QIE` into `QUE`.

The nine mapping corrections follow
[ITU-R M.1677-1, Annex 1](https://www.itu.int/dms_pubrec/itu-r/rec/m/R-REC-M.1677-1-200910-I!!PDF-E.pdf):

| Symbol | Historical code | Correct code |
| --- | --- | --- |
| A | `..-.` | `.-` |
| D | `-` | `-..` |
| F | `.-` | `..-.` |
| G | `--` | `--.` |
| I | `..-` | `..` |
| M | `-.` | `--` |
| N | `--.` | `-.` |
| T | `-..` | `-` |
| U | `..` | `..-` |

The old missing-final-LF, message/tree join, ignored arguments, fixed chunks, and
unsafe errors are deliberately corrected, not compatibility requirements. Original
mapping/output fixtures and `tests/run_baseline.sh` are archive-only; they do not
govern active corrected CLI output.

## Automated verification

`make test` runs direct C module tests, independent reference-based exhaustive
tests, allocation/I/O failure injection, and exact-byte POSIX CLI comparisons.
Tests do not derive expected characters from the active table, ignore final LFs,
or accept a crash/timeout as an expected validation failure.

The finite exhaustive domains are all 62 nonempty dot/dash sequences of lengths
1-5 (36 mapped, 26 unmapped) and all 1,296 ordered character pairs, each with and
without a word separator (2,592 cases). Five insertion permutations must produce
the same mappings and preorder. These are bounded exhaustive domains, not a claim
to have tested every possible unbounded input.

The failure matrix checks every project allocation occurrence in a successful
multi-line run, including tree construction and input/output buffer growth. Tracked
allocations must all be freed after each injected failure. Additional checks exercise
oversized growth, empty-line allocation, failed reads, permission denial, table close,
output writes/newlines, tree recursion, help, and flush errors. Test-only glibc custom
streams and GNU-compatible linker wrapping make failures deterministic even when
running as a privileged user. They are not application dependencies.

CLI cases cover every character, the English sample, the literal report input,
whitespace/separators, blank/unterminated/CRLF lines, old/new buffer boundaries,
long/many-line input, invalid codes/bytes/arguments, table-path behavior, help,
tree aliases/boundaries, later-line failure, and buffered output failure. Subprocess
timeouts are explicit, and sanitizer/coverage builds run the same complete suite.

[CI](../.github/workflows/ci.yml) has GCC/Clang jobs, sanitizer/leak checks, and
formatting/static-analysis/coverage checks. Every job and step has a timeout,
repository permissions are read-only, and checkout does not persist credentials.
Checkout is pinned to the [verified v7.0.1 release commit](https://github.com/actions/checkout/releases/tag/v7.0.1).
Runner tools were checked against the [official Ubuntu 24.04 image inventory](https://github.com/actions/runner-images/blob/main/images/ubuntu/Ubuntu2404-Readme.md).
A hosted run requires an approved commit/push; local verification is not presented
as hosted CI success.
