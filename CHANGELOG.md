# Changelog

## 0.1.0 — 2026-09-29

- Extract the NBS ANSI-stripping baseline with provenance.
- Replace whole-string parsing with a chunk-independent streaming parser.
- Strip 7-bit and 8-bit CSI, OSC, DCS, SOS, PM, and APC sequences.
- Remove terminal cursor commands without inventing display content.
- Preserve valid UTF-8 byte-for-byte.
- Prevent UTF-8 continuation bytes inside control strings from acting as ST.
- Add a stdin/file CLI with explicit errors and exit statuses.
- Add unit, adversarial, generated-property, I/O, and CLI tests.
