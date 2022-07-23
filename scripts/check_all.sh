#!/bin/bash

if ! [ -d "scripts" ]; then
    echo "run this script from the project root"
    exit 1
fi

function exit_if_failed {
    if [ "${1}" -eq 0 ]; then
        echo " passed"
    else
        echo " failed"
        exit 0
    fi
}

echo -n "Checking formatting ..."
test $(scripts/check_formatting.sh 2>&1 | wc -l) -eq 0
exit_if_failed $?

echo -n "Checking header guards ..."
python3 scripts/check_header_guards.py
exit_if_failed $?

echo -n "Checking copyright headers ..."
python3 scripts/check_copyright_headers.py
exit_if_failed $?

echo -n "Checking documentation ..."
. scripts/build_documentation.sh 1>/dev/null
exit_if_failed $?
