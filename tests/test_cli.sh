#!/bin/sh
set -eu

binary=$1
tmp_dir=${TMPDIR:-/tmp}/simple-ansi-test-$$
mkdir -p "$tmp_dir"
trap 'rm -rf "$tmp_dir"' EXIT HUP INT TERM

fail() {
    echo "FAIL: $1" >&2
    exit 1
}

printf '\033[31mred\033[0m\n' |
    "$binary" >"$tmp_dir/stdin.out" 2>"$tmp_dir/stdin.err" ||
    fail "stdin filtering returned failure"
printf 'red\n' >"$tmp_dir/expected"
cmp "$tmp_dir/expected" "$tmp_dir/stdin.out" ||
    fail "stdin filtering produced wrong bytes"
test ! -s "$tmp_dir/stdin.err" || fail "successful stdin filtering wrote stderr"

printf 'a\033]0;title\007b\n' >"$tmp_dir/input"
"$binary" "$tmp_dir/input" >"$tmp_dir/file.out" 2>"$tmp_dir/file.err" ||
    fail "file filtering returned failure"
printf 'ab\n' >"$tmp_dir/expected"
cmp "$tmp_dir/expected" "$tmp_dir/file.out" ||
    fail "file filtering produced wrong bytes"
test ! -s "$tmp_dir/file.err" || fail "successful file filtering wrote stderr"

"$binary" --help >"$tmp_dir/help" 2>"$tmp_dir/help.err" ||
    fail "--help returned failure"
grep -q '^Usage: simple-ansi' "$tmp_dir/help" || fail "--help lacks usage"
test ! -s "$tmp_dir/help.err" || fail "--help wrote stderr"

"$binary" --version >"$tmp_dir/version" 2>"$tmp_dir/version.err" ||
    fail "--version returned failure"
grep -q '^simple-ansi ' "$tmp_dir/version" || fail "--version lacks program name"
test ! -s "$tmp_dir/version.err" || fail "--version wrote stderr"

set +e
"$binary" --unknown >"$tmp_dir/bad.out" 2>"$tmp_dir/bad.err"
status=$?
set -e
test "$status" -eq 2 || fail "invalid option did not return 2"
test ! -s "$tmp_dir/bad.out" || fail "invalid option contaminated stdout"
grep -q '^simple-ansi:' "$tmp_dir/bad.err" || fail "invalid option lacks diagnostic"

set +e
"$binary" "$tmp_dir/missing" >"$tmp_dir/missing.out" 2>"$tmp_dir/missing.err"
status=$?
set -e
test "$status" -eq 1 || fail "missing input did not return 1"
test ! -s "$tmp_dir/missing.out" || fail "open failure contaminated stdout"

printf 'plain text' >"$tmp_dir/plain"
set +e
"$binary" "$tmp_dir/plain" 1>&- 2>"$tmp_dir/write.err"
status=$?
set -e
test "$status" -eq 1 || fail "closed stdout did not return 1"
grep -q 'write' "$tmp_dir/write.err" || fail "write failure lacks diagnostic"

printf 'dash file' >"$tmp_dir/-input"
(
    cd "$tmp_dir"
    "$binary" -- -input
) >"$tmp_dir/dash.out" 2>"$tmp_dir/dash.err" ||
    fail "-- did not permit a dash-prefixed filename"
printf 'dash file' >"$tmp_dir/expected"
cmp "$tmp_dir/expected" "$tmp_dir/dash.out" ||
    fail "dash-prefixed filename produced wrong bytes"

mkdir "$tmp_dir/input-directory"
set +e
"$binary" "$tmp_dir/input-directory" >"$tmp_dir/read.out" 2>"$tmp_dir/read.err"
status=$?
set -e
test "$status" -eq 1 || fail "read failure did not return 1"
test ! -s "$tmp_dir/read.out" || fail "read failure contaminated stdout"
grep -q 'read failed' "$tmp_dir/read.err" || fail "read failure lacks diagnostic"

dd if=/dev/zero bs=16383 count=1 2>/dev/null | tr '\000' a >"$tmp_dir/boundary.in"
printf '\033[31mZ\033[0m' >>"$tmp_dir/boundary.in"
dd if=/dev/zero bs=16383 count=1 2>/dev/null | tr '\000' a >"$tmp_dir/boundary.expected"
printf 'Z' >>"$tmp_dir/boundary.expected"
"$binary" "$tmp_dir/boundary.in" >"$tmp_dir/boundary.out" ||
    fail "buffer-boundary filtering returned failure"
cmp "$tmp_dir/boundary.expected" "$tmp_dir/boundary.out" ||
    fail "sequence crossing the I/O buffer boundary was corrupted"

printf '\033]0;prefix\305\234LEAK\007after\n' |
    "$binary" >"$tmp_dir/utf8-osc.out" ||
    fail "UTF-8 OSC regression returned failure"
printf 'after\n' >"$tmp_dir/expected"
cmp "$tmp_dir/expected" "$tmp_dir/utf8-osc.out" ||
    fail "UTF-8 continuation byte terminated OSC"

printf 'a\033Pprefix\305\234LEAK\033\\after\n' |
    "$binary" >"$tmp_dir/utf8-dcs.out" ||
    fail "UTF-8 DCS regression returned failure"
printf 'aafter\n' >"$tmp_dir/expected"
cmp "$tmp_dir/expected" "$tmp_dir/utf8-dcs.out" ||
    fail "UTF-8 continuation byte terminated DCS"

echo "12/12 CLI scenarios passed"
