# Historical Characterization Fixtures

These fixtures describe the unmodified implementation at commit `96d7bf0` and
the compatibility stage after roadmap phases 1-3. They are not the intended
international mapping or final output format.

- `original-mapping-2019.txt` preserves the original `src/morse.txt` bytes,
  including nine incorrect letter mappings.
- `report-example.in` is the literal input on page 3 of the original PDF.
  Its `.expected` output was reproduced during the current review, not recorded
  in the report. The runner removes the expected fixture's editor-friendly final
  LF because the original output has none.
- `message-tree-joined.*` records the known missing separator between a decoded
  message and the first preorder record. This expectation must stop governing
  the active CLI when phase 4 fixes that defect.

The 12 integration cases, including these snapshots, were verified against an
original executable compiled before code extraction. Invalid inputs were not
run against the unsafe original implementation.

SHA-256 of the original mapping:

```text
e8c102e9818bcb81381038b219187b279783fbad719c650a2c47852f0023bfd9
```
