#!/usr/bin/bash

find . -type f \( -iname "*.h" -o -iname "*.cc" \) -exec clang-format -n --style=Google {} \;
