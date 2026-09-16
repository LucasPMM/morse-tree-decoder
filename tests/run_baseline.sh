#!/bin/sh
set -eu

# All invocations are bounded. Fixtures preserve legacy bytes, including formatting defects.
binary=${1:?Usage: sh tests/run_baseline.sh BINARY}
repo_dir=$(pwd)
case "$binary" in
    /*) ;;
    *) binary="$repo_dir/$binary" ;;
esac
working_directory=${BASELINE_WORKING_DIRECTORY:-$repo_dir}
fixture_dir="$repo_dir/tests/fixtures"
timeout_seconds=${TEST_TIMEOUT:-5}
temp_dir=$(mktemp -d /tmp/morse-tree-decoder-tests.XXXXXX)
cleanup() {
    rm -f "$temp_dir/actual" "$temp_dir/expected" "$temp_dir/error" "$temp_dir/input"
    rmdir "$temp_dir"
}
trap cleanup EXIT
trap 'exit 1' HUP INT TERM
cases=0

# The runnable README example must remain identical to the verified fixture.
if ! cmp -s "$repo_dir/examples/sample.in" "$fixture_dir/words.in"; then
    printf 'FAIL: sample input differs from the verified example\n' >&2
    exit 1
fi

run_case() {
    name=$1
    format=$2
    input_format=$3
    shift 3
    input="$fixture_dir/$name.in"
    if [ "$input_format" = no-newline ]; then
        head -c -1 "$input" > "$temp_dir/input"
        input="$temp_dir/input"
    fi
    if [ "$format" = no-newline ]; then
        # Text fixtures have an editor-friendly final LF; legacy stdout does not.
        head -c -1 "$fixture_dir/$name.expected" > "$temp_dir/expected"
    else
        cp "$fixture_dir/$name.expected" "$temp_dir/expected"
    fi
    if ! (cd "$working_directory" && timeout "$timeout_seconds" "$binary" "$@") \
        < "$input" > "$temp_dir/actual" 2> "$temp_dir/error"; then
        printf 'FAIL: %s (execution failed)\n' "$name" >&2
        cat "$temp_dir/error" >&2
        exit 1
    fi
    if [ -s "$temp_dir/error" ] || ! cmp -s "$temp_dir/actual" "$temp_dir/expected"; then
        printf 'FAIL: %s (stdout/stderr mismatch)\n' "$name" >&2
        diff -u "$temp_dir/expected" "$temp_dir/actual" >&2 || true
        cat "$temp_dir/error" >&2
        exit 1
    fi
    cases=$((cases + 1))
}

run_case unaffected-letters no-newline normal
run_case digits no-newline normal
run_case words no-newline normal
run_case multiple-lines no-newline normal
run_case blank-lines no-newline normal
run_case all-legacy-symbols no-newline normal
run_case historical/report-example no-newline normal
run_case words no-newline no-newline
run_case tree-only exact normal -a
run_case historical/message-tree-joined exact normal -a
run_case words no-newline normal ignored-option
run_case words no-newline normal -a extra

printf 'Baseline integration tests passed (%s cases).\n' "$cases"
