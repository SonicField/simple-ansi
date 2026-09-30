# Contract verification

This document maps each externally observable claim in
[`CONTRACT.md`](CONTRACT.md) to evidence that would fail if the behavior
changed. Assertion names refer to `tests/test_strip_ansi.c` unless another
suite is named.

## Command line and I/O

| Contract claim | Direct evidence |
|---|---|
| Read standard input or one named file | stdin and file scenarios in `tests/test_cli.sh` |
| Accept a dash-prefixed filename after `--` | dash-prefixed filename scenario in `tests/test_cli.sh` |
| Reject invalid options and multiple inputs with status 2 | invalid-option scenarios in `tests/test_cli.sh` |
| Write results only to stdout and diagnostics only to stderr | stream checks in every CLI success and failure scenario |
| Return status 1 for open, read, and write failures | missing-file, directory-read, and closed-stdout scenarios |
| Detect closed stdout and broken pipes for help and version output | four help/version failure scenarios in `tests/test_cli.sh` |
| Retry interrupted reads and writes and complete short writes | `short and interrupted I/O completes`; `short writes preserve exact output` |
| Report permanent and zero-length I/O failures | three failure checks in `tests/test_io.c` |

## Filtering

| Contract claim | Direct evidence |
|---|---|
| Remove 7-bit and 8-bit CSI | `CSI SGR removed`; `8-bit CSI` |
| Remove OSC terminated by BEL, 7-bit ST, or 8-bit ST | `OSC+BEL removed`; `OSC+ST removed`; `8-bit OSC` |
| Remove 7-bit and 8-bit DCS, SOS, PM, and APC | the ten named family assertions |
| Remove ordinary ECMA-48 escape sequences | `simple escape removed`; `bare ESC at end removed` |
| Remove C0 except tab/newline, DEL, and standalone C1 | `control chars stripped, nl/tab kept`; `DEL stripped`; `C1 controls stripped` |
| Preserve valid UTF-8, including continuation bytes in the C1 range | all 1,111,936 valid non-ASCII scalar values in `tests/test_utf8.c`; `B16: U+0080 ... preserved` |
| Preserve other bytes outside control ranges without transcoding | `binary and malformed UTF-8 policy` |
| Treat UTF-8 continuation `0x9c` inside a string as payload | all UTF-8/ST split assertions; both CLI regression scenarios |
| CAN and SUB cancel CSI or control strings | `CAN cancels DCS`; `SUB cancels OSC` |

## Malformed input and streaming

| Contract claim | Direct evidence |
|---|---|
| Discard incomplete ESC, CSI, and control strings through end of input | `truncated ESC discarded`; `truncated CSI discarded`; `truncated OSC discarded` |
| Discard non-final CSI bytes until a final or cancellation | malformed CSI cases in `tests/test_strip_ansi.c` |
| Keep a non-terminating ESC and following content inside a control string | control-string family and split-boundary assertions |
| Reconsider an invalid UTF-8 continuation as string syntax | `malformed UTF-8 does not hide ST` |
| Produce the same bytes for every tested chunk division | all representative split assertions and 1,000 deterministic property cases |
| Impose no line-sized buffer and handle large streams | the partitioned 1 MiB property case |
| Never invent bytes or expand output | generated subsequence and non-expansion properties |
| Report invalid C API arguments and calls after finish | the nine API lifecycle assertions |
| Support exact in-place filtering for NUL-terminated strings | inherited `strip_ansi` assertions and length checks |

The statements that this is not a terminal emulator or a general security
boundary are scope constraints, not executable behavior claims.
