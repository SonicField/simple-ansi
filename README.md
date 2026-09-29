# simple-ansi

A small streaming command-line filter that removes terminal control sequences
while preserving the remaining bytes in their original order.

```console
$ printf '\033[31mred\033[0m\n' | simple-ansi
red
```

It is useful for cleaning compiler output, logs, copied terminal output, and
ephemeral text produced by AI-driven command-line workflows.

## Why this tool?

`simple-ansi` does one job. It has no runtime dependencies, does not need a
terminal, and can process an arbitrarily chunked stream without buffering the
whole input.

It deliberately does not emulate a screen. Cursor movement is removed; it is
not guessed into spaces or used to rewrite prior text. If the visual terminal
screen matters, use
[`simple-termshot`](https://github.com/SonicField/simple-termshot) instead.

## Usage

Read standard input:

```sh
some-command | simple-ansi >clean.txt
```

Read one file:

```sh
simple-ansi captured-output.txt
```

Use `--` for a filename beginning with a dash:

```sh
simple-ansi -- -capture
```

Other commands:

```sh
simple-ansi --help
simple-ansi --version
```

Successful output goes only to standard output. Diagnostics go to standard
error. Exit status is 0 on success, 1 for an input/output failure, and 2 for
invalid usage.

## Build and test

A C11 compiler, POSIX environment, and `make` are required.

```sh
make
make test
make sanitize CC=clang
make analyze ANALYZER_CC=gcc
```

Install under `/usr/local`:

```sh
sudo make install
```

Or select another prefix:

```sh
make install PREFIX="$HOME/.local"
```

Packaging can stage without writing to the final filesystem:

```sh
make install DESTDIR=/tmp/simple-ansi-package
```

## What is removed?

The filter handles 7-bit and 8-bit forms of CSI, OSC, DCS, SOS, PM, and APC,
ordinary ECMA-48 escape sequences, C0 controls other than tab/newline, DEL, and
standalone C1 controls. Valid UTF-8 passes byte-for-byte. Parser state is
retained when any sequence or UTF-8 character crosses a read boundary.

The precise behaviour, malformed-input policy, C API, and exit statuses are in
[`docs/CONTRACT.md`](docs/CONTRACT.md). Extraction provenance and inherited
baseline evidence are in [`docs/EXTRACTION.md`](docs/EXTRACTION.md).

## Limits and security

This is a linear byte filter, not a terminal emulator and not a general-purpose
security boundary. It does not apply backspaces, carriage returns, erasure, or
cursor motion; it removes their controls and retains surrounding text.
It preserves Unicode—including bidirectional formatting characters—and does
not make untrusted data safe for HTML, shell evaluation, or another parser.

An unterminated CSI or control string is discarded through end of input. This
prevents its apparent payload from leaking but can remove trailing text from
malformed input.

## Licence

MIT. Copyright © 2026 Alex Turner.
