#!/usr/bin/env bash

set -eo pipefail

find include/ src/ test/ -type f \( -iname "*.h" -o -iname "*.cc" \) -exec clang-format -n {} \; &> /tmp/checks.txt
cat /tmp/checks.txt
[[ ! -s /tmp/checks.txt ]]
