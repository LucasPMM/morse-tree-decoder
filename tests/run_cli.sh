#!/bin/sh
set -eu

binary=${1:?Usage: sh tests/run_cli.sh BINARY}
repo_dir=$(pwd)
case "$binary" in
    /*) ;;
    *) binary="$repo_dir/$binary" ;;
esac
working_directory=$repo_dir
fixture_dir="$repo_dir/tests/fixtures"
timeout_seconds=${TEST_TIMEOUT:-5}
temp_dir=$(mktemp -d /tmp/morse-tree-decoder-cli.XXXXXX)
cleanup() {
    rm -f "$temp_dir/input" "$temp_dir/expected" "$temp_dir/actual" "$temp_dir/error"
    rmdir "$temp_dir"
}
trap cleanup EXIT
trap 'exit 1' HUP INT TERM
cases=0

fail() {
    printf 'FAIL: %s\n' "$1" >&2
    cat "$temp_dir/error" >&2
    diff -u "$temp_dir/expected" "$temp_dir/actual" >&2 || true
    exit 1
}

run_success() {
    name=$1
    shift
    if ! (cd "$working_directory" && timeout "$timeout_seconds" "$binary" "$@") \
        < "$temp_dir/input" > "$temp_dir/actual" 2> "$temp_dir/error"; then
        fail "$name (execution failed)"
    fi
    if [ -s "$temp_dir/error" ] || ! cmp -s "$temp_dir/expected" "$temp_dir/actual"; then
        fail "$name (byte mismatch)"
    fi
    cases=$((cases + 1))
}

run_failure() {
    name=$1
    reason=$2
    shift 2
    if (cd "$working_directory" && timeout "$timeout_seconds" "$binary" "$@") \
        < "$temp_dir/input" > "$temp_dir/actual" 2> "$temp_dir/error"; then
        fail "$name (unexpected success)"
    else
        status=$?
    fi
    if [ "$status" -ne 1 ] || ! grep -Fq -- "$reason" "$temp_dir/error" || \
        ! cmp -s "$temp_dir/expected" "$temp_dir/actual"; then
        fail "$name (incorrect failure or partial decoded line)"
    fi
    cases=$((cases + 1))
}

fixture() {
    name=$1
    input_name=$2
    expected_name=$3
    shift 3
    cp "$fixture_dir/$input_name" "$temp_dir/input"
    cp "$fixture_dir/$expected_name" "$temp_dir/expected"
    run_success "$name" "$@"
}

# Active expectations are never normalized: final LF is part of the new contract.
cmp -s "$repo_dir/examples/sample.in" "$fixture_dir/words.in"
fixture unaffected-letters unaffected-letters.in unaffected-letters.expected
fixture digits digits.in digits.expected
fixture all-symbols all-symbols.in all-symbols.expected
fixture words words.in words.expected
fixture multiple-lines multiple-lines.in multiple-lines.expected
fixture blank-lines blank-lines.in blank-lines.expected
fixture report-example historical/report-example.in report-example.expected
fixture empty-tree tree-only.in tree-only.expected -a
fixture empty-tree-alias tree-only.in tree-only.expected --print-tree
fixture help tree-only.in help.expected --help

head -c -1 "$fixture_dir/words.in" > "$temp_dir/input"
cp "$fixture_dir/words.expected" "$temp_dir/expected"
run_success unterminated-line
printf ' \t...   \t--- ... \t/ .... . .-.. .--. \t\n' > "$temp_dir/input"
printf 'SOS HELP\n' > "$temp_dir/expected"
run_success horizontal-whitespace
printf ' / / ... / / --- / \n' > "$temp_dir/input"
printf '  S  O \n' > "$temp_dir/expected"
run_success repeated-separators
printf '/\n' > "$temp_dir/input"
printf ' \n' > "$temp_dir/expected"
run_success standalone-separator
printf '\t  \n' > "$temp_dir/input"
printf '\n' > "$temp_dir/expected"
run_success whitespace-only-line
printf '' > "$temp_dir/input"
printf '' > "$temp_dir/expected"
run_success empty-input
printf '... --- ...\r\n.-\r\n\r\n' > "$temp_dir/input"
printf 'SOS\nA\n\n' > "$temp_dir/expected"
run_success crlf-lines
printf '... --- ...\n' > "$temp_dir/input"
printf 'SOS\n' > "$temp_dir/expected"
cat "$fixture_dir/tree-only.expected" >> "$temp_dir/expected"
run_success message-tree-boundary -a
run_success message-tree-boundary-alias --print-tree

for length in 1 9 10 11 126 127 128 129 254 255 256 257 498 499 500 501 1023 1024 1025 4095 4096 4097 20000; do
    awk -v n="$length" 'BEGIN {for (i=0;i<n;i++) printf "%s", i%2==0 ? "." : " "; printf "\n"}' > "$temp_dir/input"
    awk -v n="$length" 'BEGIN {for (i=0;i<n;i+=2) printf "E"; printf "\n"}' > "$temp_dir/expected"
    run_success "line-size-$length"
done
awk 'BEGIN {for (i=0;i<10000;i++) print "."}' > "$temp_dir/input"
awk 'BEGIN {for (i=0;i<10000;i++) print "E"}' > "$temp_dir/expected"
run_success many-lines

printf '' > "$temp_dir/expected"
for token in x /x /. '.../---' -x- ------ '..--' '...#' 3 a; do
    printf '%s\n' "$token" > "$temp_dir/input"
    run_failure "invalid-token-$token" 'line 1 at token 1'
done
for byte in '\000' '\001' '\007' '\033' '\177' '\200' '\377'; do
    printf '. %b\n' "$byte" > "$temp_dir/input"
    run_failure "invalid-byte-$byte" 'unsupported byte'
done
printf '.\r' > "$temp_dir/input"
run_failure bare-cr-at-eof 'unknown or invalid Morse token'
printf '\r.\n' > "$temp_dir/input"
run_failure internal-cr 'unknown or invalid Morse token'
printf '.\nx\n' > "$temp_dir/input"
printf 'E\n' > "$temp_dir/expected"
run_failure invalid-later-line 'line 2 at token 1' -a
printf '' > "$temp_dir/expected"
for option in --unknown -h input.txt -- -aextra; do
    run_failure "unknown-option-$option" 'Unknown option' "$option"
done
run_failure extra-option 'Expected at most one option' -a --help
run_failure extra-help-argument 'Expected at most one option' --help extra
run_failure repeated-option 'Expected at most one option' -a -a

# Help/argument parsing does not require the mapping; decoding deliberately does.
working_directory=$temp_dir
cp "$fixture_dir/help.expected" "$temp_dir/expected"
run_success help-outside-repository --help
printf '' > "$temp_dir/expected"
run_failure table-path-contract 'file could not be opened'
run_failure argument-validation-before-table 'Unknown option' --unknown
working_directory=$repo_dir

if [ -e /dev/full ]; then
    cp "$fixture_dir/words.in" "$temp_dir/input"
    if timeout "$timeout_seconds" "$binary" < "$temp_dir/input" > /dev/full 2> "$temp_dir/error"; then
        fail 'buffered-output-failure (unexpected success)'
    else
        status=$?
    fi
    [ "$status" -eq 1 ] && grep -Fq 'Cannot flush output' "$temp_dir/error" || fail 'buffered-output-failure'
    cases=$((cases + 1))
fi

printf 'CLI integration tests passed (%s cases).\n' "$cases"
