#!/usr/bin/bash

CONF="doc/DoxyConf"

if ! command -v doxygen &>/dev/null; then
    echo "Cannot build documentation because doxygen is missing"
    exit 1
fi

if ! [ -f "${CONF}" ]; then
    echo "DoxyConf file missing"
    echo "Run this script from the project root"
    exit 1
fi

doxygen ${CONF} .
