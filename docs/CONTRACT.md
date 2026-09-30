# `simple-ansi` contract

## Purpose

`simple-ansi` is a byte-stream filter. It removes terminal control sequences
from a linear stream and writes the remaining bytes in their original order.
It does not emulate a terminal screen, move a cursor, erase previously written
text, or infer spaces. Use `simple-termshot` when screen reconstruction is
required.

## Command line

```text
simple-ansi [--help] [--version] [FILE]
```

- With no `FILE`, input is read from standard input.
- With one `FILE`, input is read from that file.
- Filtered bytes are written to standard output.
- Diagnostics are written to standard error and never to standard output.
- Options and filenames cannot be combined.
- More than one input file is an error.
- A filename beginning with `-` can be supplied after `--`.

Exit status is `0` on success, `1` after an input or output failure, and `2`
for invalid command-line usage.

## Filtering

The filter recognises these ECMA-48 forms across arbitrary input chunk
boundaries:

| Family | 7-bit introducer | 8-bit introducer | Terminator |
| --- | --- | --- | --- |
| CSI | `ESC [` | `0x9b` | first final byte `0x40`–`0x7e` |
| OSC | `ESC ]` | `0x9d` | BEL, `ESC \\`, or `0x9c` |
| DCS | `ESC P` | `0x90` | `ESC \\` or `0x9c` |
| SOS | `ESC X` | `0x98` | `ESC \\` or `0x9c` |
| PM | `ESC ^` | `0x9e` | `ESC \\` or `0x9c` |
| APC | `ESC _` | `0x9f` | `ESC \\` or `0x9c` |

It also removes:

- ECMA-48 escape sequences consisting of `ESC`, zero or more intermediate
  bytes (`0x20`–`0x2f`), and a final byte (`0x30`–`0x7e`);
- C0 control bytes except horizontal tab (`0x09`) and line feed (`0x0a`);
- DEL (`0x7f`); and
- standalone C1 control bytes (`0x80`–`0x9f`).

Valid RFC 3629 UTF-8 sequences pass byte-for-byte, including continuation
bytes in the C1 range. All other bytes outside the control ranges pass
byte-for-byte. The filter does not otherwise validate or transcode text.
UTF-8 is also recognised inside discarded control-string payloads. A valid
continuation byte equal to 8-bit ST (`0x9c`) is payload, not a terminator.

CAN (`0x18`) and SUB (`0x1a`) cancel an in-progress CSI or control string.
They are themselves removed. Subsequent input is handled normally.

## Malformed and truncated input

Safety takes precedence over recovering possible text from an apparent
control sequence:

- A CSI ends only at its final byte or at CAN/SUB.
- A control string ends only at its documented terminator or at CAN/SUB.
- An incomplete escape, CSI, or control string at end of input is discarded.
- A non-final byte inside CSI is discarded, even when it is outside the
  formal parameter/intermediate ranges.
- An ESC inside a control string only terminates it when followed by `\\`;
  otherwise it and the following content remain part of the string.
- If a UTF-8 leading byte inside a control string is not followed by a valid
  continuation byte, the candidate sequence is abandoned. That next byte is
  reconsidered as control-string syntax, so an invalid continuation `0x9c`
  terminates the string as ST.

These rules prevent unterminated control payloads from leaking, at the cost of
discarding trailing text after a malformed introducer.

## Streaming guarantees

For a fixed byte stream, output is independent of how the caller divides the
input into chunks. The filter keeps only parser state between chunks; it does
not impose a line-size limit. Output never contains bytes invented by the
filter and is never longer than input.

The C API reports invalid arguments and a previously finished state. The CLI
retries interrupted reads and writes, completes short writes, and reports
permanent I/O failures with non-zero status.

## Deliberate limits

- This is not a terminal emulator or sanitiser for interactive terminal use.
- Removing controls does not make untrusted text safe for HTML, shells,
  programming languages, or other non-terminal contexts.
- The filter does not interpret backspace, carriage return, cursor movement,
  erase commands, overstriking, or alternate-screen state.
- It does not normalise Unicode or repair invalid UTF-8.
