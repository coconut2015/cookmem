#!/bin/bash
#
# Run the performance tests and generate a markdown report.
#
# Each test is run RUNS times (default 5).  For each column, the best and the
# worst times are dropped and the remaining times are averaged.
#
# Usage: ./perftest.sh [build directory] [report file]
#
#   build directory     where perf_cookmem_* executables are (default: release)
#   report file         optional file to also save the markdown report to
#
# The report is printed to stdout, ready to paste into README.md.
# Progress messages go to stderr.

BIN_DIR="${1:-release}"
REPORT="$2"
RUNS="${RUNS:-5}"

declare -A DESCRIPTIONS=(
    [perf_cookmem_1]="Copy a 200-character string into a vector 1M times (small bins, plus large vector growth)"
    [perf_cookmem_2]="Same as perf_cookmem_1, then clear and fill again (reuse of freed chunks)"
    [perf_cookmem_3]="10k strings of growing length 1 to 10000 (mostly tree bins)"
    [perf_cookmem_4]="Same as perf_cookmem_3, then clear and build again (reuse of freed tree chunks)"
    [perf_cookmem_5]="Small same-size churn (\`std::list<int>\`, small bins)"
    [perf_cookmem_6]="Mixed sizes (\`std::map<int, std::string>\`, values 0-999 chars, erase and re-insert)"
    [perf_cookmem_7]="50k free chunks of the same 1KB size, then merging with free neighbors"
    [perf_cookmem_8]="Fragmentation and best-fit search (sizes 256 B-8 KB, refill the holes)"
    [perf_cookmem_9]="Interleaved \`realloc\` growth up to 32KB"
    [perf_cookmem_10]="Large blocks (128KB-4MB) going straight to the arena"
    [perf_cookmem_11]="Sliding window (FIFO) of 1024 live blocks, 16-2047 bytes"
    [perf_cookmem_12]="10k vectors grown in interleaved order"
)

# Read one CSV line per run (dlmalloc,rb,avl) from stdin.  For each column,
# drop the best and the worst values (when there are at least 3 runs) and
# print the average of the rest.
trimmed_average ()
{
    awk -F, '
    {
        for (c = 1; c <= NF; ++c)
            v[c, NR] = $c
        if (NF > cols)
            cols = NF
    }
    END {
        for (c = 1; c <= cols; ++c)
        {
            n = 0
            for (r = 1; r <= NR; ++r)
                a[++n] = v[c, r]
            # insertion sort
            for (i = 2; i <= n; ++i)
            {
                x = a[i]
                for (j = i - 1; j >= 1 && a[j] > x; --j)
                    a[j + 1] = a[j]
                a[j + 1] = x
            }
            lo = 1; hi = n
            if (n >= 3) { lo = 2; hi = n - 1 }
            sum = 0
            for (i = lo; i <= hi; ++i)
                sum += a[i]
            printf "%s%.6f", (c > 1 ? "," : ""), sum / (hi - lo + 1)
        }
        printf "\n"
    }'
}

TESTS=$(ls "$BIN_DIR"/perf_cookmem_* 2>/dev/null | sort -V)
if [ -z "$TESTS" ]; then
    echo "No perf_cookmem_* executables found in $BIN_DIR" >&2
    exit 1
fi

TABLE="| Test | What it covers | dlmalloc | CookMem (RB-tree) | CookMem (AVL-tree) |
|------|----------------|---------:|------------------:|-------------------:|"

for t in $TESTS; do
    name=$(basename "$t")
    echo "Running $name ($RUNS runs)..." >&2

    results=""
    failed=0
    for ((i = 0; i < RUNS; ++i)); do
        line=$("$t")
        if [ $? -ne 0 ]; then
            failed=1
            break
        fi
        results+="$line"$'\n'
    done

    if [ $failed -ne 0 ]; then
        echo "  $name failed" >&2
        TABLE+=$'\n'"| $name | ${DESCRIPTIONS[$name]} | failed | failed | failed |"
        continue
    fi

    IFS=, read -r dl rb avl <<< "$(printf "%s" "$results" | trimmed_average)"
    TABLE+=$'\n'"| $name | ${DESCRIPTIONS[$name]} | $dl | $rb | $avl |"
done

REPORT_TEXT="Each test was run $RUNS times. For each column, the best and the worst
times were dropped and the remaining times were averaged. The numbers are
in seconds.

$TABLE"

echo >&2
if [ -n "$REPORT" ]; then
    echo "$REPORT_TEXT" > "$REPORT"
    echo "Report saved to $REPORT" >&2
    echo >&2
fi
echo "$REPORT_TEXT"
