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

echo "8/8 CLI scenarios passed"
