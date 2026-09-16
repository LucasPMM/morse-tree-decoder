# Decoding Contract

This project is becoming **Morse Tree Decoder** (`morse-tree-decoder`). It remains
a C command-line decoder based on a file-backed binary trie. A dot selects the
left child and a dash selects the right child. The root and intermediate nodes
may be empty; only mapped nodes represent decoded characters.

This document distinguishes the transitional implementation after roadmap phases
1-3 from the intended phase 4 behavior. Snapshot tests of historical defects are
temporary characterization tests, not a promise to preserve those defects forever.

## Current compatibility stage

### Build, execution, and mapping location

Build from the repository root with `make`, then execute:

```sh
./morse < examples/sample.in
./morse -a < examples/sample.in
```

The executable name remains `morse`; the selected repository name does not rename
the binary or change the remote yet. The application loads `data/morse.txt`
relative to its working directory, so these commands must run from the repository
root. There is no automatic search for the table beside the executable, no
`--table` option, and no dependency on the old `src/` working directory.

The table module accepts an explicit path independently of the CLI. Compilation
may override `MORSE_TABLE_PATH` using `CPPFLAGS` when a different deployment path
is necessary; the default is intentionally not an absolute build-machine path.

The application currently recognizes `-a` only when it is the sole argument.
Other arguments are ignored, matching the original program. Help, long aliases,
and strict option validation are deferred to phase 4.

### Transitional mapping and input

- The bundled table has 26 uppercase letters and 10 digits. Its nine incorrect
  letter mappings are retained until phase 4 to separate algorithm extraction
  from functional corrections.
- A table record contains a single-byte symbol and a nonempty dot/dash code.
  This stage retains the old maximum of nine signals, with bounded parsing.
  Blank lines are skipped. Repeated codes overwrite the earlier symbol through
  the trie insertion API; duplicate-table validation is not complete yet.
- Spaces separate message tokens. A standalone `/` produces exactly one space,
  including leading, trailing, and consecutive separator tokens. Tabs are not
  token delimiters yet.
- Messages are read using the original 500-byte input buffer. Each successful
  `fgets` call forms a chunk of at most 499 bytes, not necessarily a complete
  physical line. Only a final LF is removed. CRLF, embedded NUL, and long-line
  behavior are not part of the supported compatibility input.
- The line decoder borrows an immutable string and a caller-owned output buffer.
  It performs no stream I/O or allocation, returns a status, and identifies a
  failing token by its one-based index. Its output is only usable on success.

### Transitional output and failures

For supported valid input, decoded chunks are separated by one LF, with no
additional LF after the last chunk. Empty input emits nothing. Blank input chunks
are preserved. `-a` appends mapped nodes in preorder (root, left, right), one
`symbol code` record per line, omitting empty nodes.

The last decoded chunk is still joined directly to the first tree record. For
example, `... --- ...` with `-a` begins with `SOSE .`, not two separate lines.
This is a documented format defect scheduled for phase 4.

Safe module boundaries already check table opening/parsing, allocations, failed
lookups, buffer capacity, and basic stream errors. Failures produce English
diagnostics on standard error and a nonzero exit status. Partial trees are freed.
Invalid message chunks are decoded before any of their output is printed; earlier
successful chunks may already have been emitted. The tree is omitted on a failure.
Undefined behavior, permissive malformed parsing, and crashes are not compatibility
requirements. Complete validation and deterministic failure injection remain later work.

## Intended phase 4 contract

These roadmap defaults define the next phase; they are not all implemented in the
current stage. Completing phases 1-3 does not execute or authorize later phases.

1. Ship international mappings for uppercase `A-Z` and `0-9`, with no lowercase,
   punctuation, accented characters, encoder, or legacy CLI mode.
2. Keep `morse` and `-a`; add English `--help` and `--print-tree`. Reject unknown
   options and extra arguments with a nonzero exit status.
3. Decode complete physical lines using checked dynamic buffer growth. Accept LF,
   CRLF, and EOF without a final newline. Every decoded line, including an empty
   line, has one output LF. Empty input still emits nothing.
4. Treat spaces and tabs as token delimiters. A standalone `/` emits one space;
   leading, trailing, and repeated separators are allowed. Attached forms such
   as `.../---` are invalid. No comment syntax is added.
5. Reject invalid signals, unknown codes, embedded NUL, and unsupported control
   bytes. Do not reinterpret them as dashes or guess a character.
6. Finish decoding each line before writing it. On error, omit that line and the
   tree, report its line/token in English, and exit nonzero. Previous valid lines
   may already have been printed; the whole input is not buffered.
7. Print the tree only after successful decoding, with each record on a separate
   line and without joining it to the last message.
8. Retain the repository-root `data/morse.txt` default and diagnose missing files.
   Arbitrary-working-directory resolution and `--table` remain out of scope.
9. Restrict mapping symbols to uppercase letters and digits, and codes to at most
   five signals for this project's supported alphabet. Reject empty tables,
   malformed records, duplicates of either symbol or code, and oversized fields.
   Accept blank lines, record whitespace, LF/CRLF, and an unterminated final record.
   Do not require a custom table to contain all 36 symbols.
10. Check allocation growth arithmetic and read, write, flush, and relevant close
    failures; release all owned state on every exit path.

The low-level insertion operation may continue to replace a code's value; the
table loader, not the trie, enforces uniqueness in a file. This keeps the original
data-structure operation and the stricter file contract separate.

## Historical evidence and deliberate corrections

The [unchanged 2019 report](original-report-2019.pdf) describes trie construction,
lookup, a literal input example, preorder printing, and theoretical complexity.
It contains no enumerated test suite, timing measurements, or expected output for
that example. The archived PDF is intentionally preserved in its original language.

The [archived diagram](figures/morse-tree.png) was already tracked in the original
repository and also appears in the PDF. Its external origin/license is not stated
there; no new attribution or license is invented. It is an extended Morse chart
with punctuation, accented letters, and multi-character symbols that this program
does not support, not an exact rendering of the original buggy table.

The exact report input is kept in
`tests/fixtures/historical/report-example.in`:

```text
--- ... / -. . - --- ... / --.- .. .
```

The unmodified baseline compiled from commit `96d7bf0` was exercised during this
refactoring review, and emitted `OS MEDOS QUE`, without a final LF. The extracted
implementation must currently produce the same bytes. This output was reproduced
now, not recorded in the 2019 PDF. With international mappings the unchanged input
will decode to `OS NETOS QIE`; that is a reference expectation until phase 4 runs.
Do not edit the input to silently change `QIE` into `QUE`.

The mapping corrections follow
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

## Tests and next-phase migration

`make test` runs direct C module tests and bounded POSIX CLI characterization
tests. Integration comparisons check exact stdout bytes, empty stderr, and a zero
exit status. Every invocation has a timeout. The same 12 CLI cases were run
successfully against the original executable before accepting the extracted code.

Expected text fixtures end with LF for editors. The test runner explicitly removes
only that final byte where the legacy output must lack a final LF. Tree fixtures
are compared without normalization, including the known message/tree boundary defect.

| Planned rule | Current evidence | Phase 4/5 follow-up |
| --- | --- | --- |
| 36 mappings and trie direction | Legacy alphabet/digits, insertion/lookup tests | Independent corrected oracle, all 62 short codes |
| Word separators and whitespace | English words, repeated spaces/separators | Tabs, CRLF, invalid attached separators |
| Physical lines and output LF | Multiline, blank, unterminated input snapshots | Dynamic boundaries, final LF correction |
| CLI and preorder | Empty-input tree, joined-output and argument snapshots | Help, alias, strict arguments, corrected boundary |
| Mapping files | Original bytes, malformed/missing-table checks | Empty/duplicate/oversized records, control bytes |
| Ownership and failures | Placeholder initialization, statuses, sanitizer target | Allocation/I/O injection, overflow, exhaustive cleanup |

In phase 4, move defect snapshots to archive-only characterization checks and
replace active CLI expectations with the corrected contract. Keep the original
mapping fixture unchanged. Do not weaken tests by ignoring newlines or generating
expected characters from the active mapping file.
