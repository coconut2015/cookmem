#!/bin/bash
#
# Compare the performance tests of two release builds, such as master and a
# branch, and print a markdown table of the changes.
#
# Usage: bash perfcompare.sh <base build directory> <new build directory>
#
# The builds are run alternately, ROUNDS times each (default 2), so that
# both see the same changes in machine load.  Each round runs perftest.sh,
# which runs each test RUNS times (default 5) and drops the best and the
# worst times.  The table shows the average of the rounds.
#
# The dlmalloc column is the same code in both builds, so its change shows
# how much the times vary between runs.

BASE_DIR="$1"
NEW_DIR="$2"
ROUNDS="${ROUNDS:-2}"
PERFTEST="$(dirname "$0")/perftest.sh"

if [ -z "$BASE_DIR" ] || [ -z "$NEW_DIR" ]; then
    echo "Usage: $0 <base build directory> <new build directory>" >&2
    exit 1
fi

RESULTS=$(mktemp)
trap 'rm -f "$RESULTS"' EXIT

for ((r = 1; r <= ROUNDS; ++r)); do
    for side in base new; do
        if [ "$side" = base ]; then dir="$BASE_DIR"; else dir="$NEW_DIR"; fi
        echo "Round $r of $ROUNDS: $side ($dir)" >&2
        # Rows look like: | perf_cookmem_1 | description | dl | rb | avl |
        if ! DATA_FILE= bash "$PERFTEST" "$dir" 2> /dev/null > "$RESULTS.out"; then
            echo "perftest.sh failed for $dir" >&2
            exit 1
        fi
        grep '^| perf_' "$RESULTS.out" |
            awk -F'|' -v side="$side" '{
                for (i = 2; i <= 6; ++i) gsub(/^ +| +$/, "", $i)
                print side "," $2 "," $4 "," $5 "," $6
            }' >> "$RESULTS"
    done
done
rm -f "$RESULTS.out"

echo "Each build was run in $ROUNDS rounds, alternately, with perftest.sh"
echo "(${RUNS:-5} runs per test, best and worst dropped).  Times are in seconds."
echo "A negative change is faster."
echo
awk -F, '
function change(b, n)
{
    if (b == "" || n == "" || b + 0 == 0)
        return "n/a"
    return sprintf ("%+.1f%%", (n - b) * 100 / b)
}
function avg(key)
{
    return (count[key] > 0) ? sum[key] / count[key] : ""
}
{
    if (!($2 in seen))
    {
        seen[$2] = 1
        names[++n] = $2
    }
    for (c = 3; c <= 5; ++c)
    {
        # A failed test reports "failed" instead of a time.
        if ($c ~ /^[0-9.]+$/)
        {
            sum[$1, $2, c] += $c
            ++count[$1, $2, c]
        }
    }
}
END {
    print "| Test | dlmalloc | change | RB-tree base | RB-tree new | change | AVL-tree base | AVL-tree new | change |"
    print "|------|---------:|---:|---------:|---------:|---:|---------:|---------:|---:|"
    for (i = 1; i <= n; ++i)
    {
        t = names[i]
        line = "| " t
        for (c = 3; c <= 5; ++c)
        {
            b = avg(("base" SUBSEP t SUBSEP c))
            v = avg(("new" SUBSEP t SUBSEP c))
            if (c == 3)
                line = line sprintf (" | %s | %s", (v == "" ? "failed" : sprintf ("%.6f", v)), change(b, v))
            else
                line = line sprintf (" | %s | %s | %s", (b == "" ? "failed" : sprintf ("%.6f", b)),
                                     (v == "" ? "failed" : sprintf ("%.6f", v)), change(b, v))
        }
        print line " |"
    }
}' "$RESULTS"
