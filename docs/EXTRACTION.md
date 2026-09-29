# Extraction record

The initial implementation was extracted from NBS Framework commit
`a1084a3aaab1d933585438b2061ec8c86150f1c2`.

The two source copies at that revision were byte-identical:

- `src/nbs-sidecar/strip_ansi.c`
- `src/nbs-ts-tools/strip_ansi.c`
- SHA-256: `d33d465327d82b3cd59a5c14d7fbafea20598031937be21e048bceb3027d2ff4`

The inherited test was `tests/test_sidecar_strip_ansi_unit.c`. Its 59
assertions passed before extraction with:

```sh
gcc -Wall -Wextra -Wshadow -Werror -std=c11 \
  -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE \
  -Isrc/nbs-sidecar -Isrc/nbs-common \
  -o /tmp/simple-ansi-nbs-baseline \
  tests/test_sidecar_strip_ansi_unit.c src/nbs-sidecar/strip_ansi.c
/tmp/simple-ansi-nbs-baseline
```

The source, header, assertion helper, and tests were imported under the same
MIT licence. Include paths and the test filename were adjusted for this
standalone repository.

The inherited behaviour is only a baseline. In particular, it expands some
cursor-right sequences into spaces and does not parse complete 8-bit control
sequences. Later commits replace those NBS-specific and incomplete behaviours
under an explicit standalone contract.
