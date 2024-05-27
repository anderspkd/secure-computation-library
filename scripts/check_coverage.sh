#!/usr/bin/env bash

set -o pipefail

readarray -t COV <<< $(grep 'Overall coverage rate:' $1 -A 2)

LINES=$(echo ${COV[1]} | grep -oE '[0-9]+\.[0-9]+')
FUNCS=$(echo ${COV[2]} | grep -oE '[0-9]+\.[0-9]+')

echo "Line coverage:      ${LINES}% (target: ${COV_THRESHOLD_LINES}%)"
echo "Function coverage:  ${FUNCS}% (target: ${COV_THRESHOLD_FUNCS}%)"

awk "BEGIN { if (${LINES} < ${COV_THRESHOLD_LINES}) { print \"line coverage not met\";  exit 1 } }"
awk "BEGIN { if (${FUNCS} < ${COV_THRESHOLD_FUNCS}) { print \"function coverage not met\";  exit 1 } }"
