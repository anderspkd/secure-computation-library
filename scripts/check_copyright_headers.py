#!/usr/bin/env python3

import os

header = """\
/* SCL --- Secure Computation Library
 * ---- THIS LINE IS IGNORED ----
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
"""

copyright_line = " * Copyright (C) 2023"

def check_file(filename, path):
    expected_header = header.rstrip().split("\n")
    with open(path, 'r') as f:
        lines = f.readlines()[:len(expected_header) + 1]
        n = 0
        good = True
        for a, b in zip(expected_header, lines):
            ## copyright line
            if n == 1:
                if not b.startswith(copyright_line):
                    good = False
                    break
            elif a.rstrip() != b.rstrip():
                good = False
                break
            n += 1
        ## license must be followed by an empty line
        good = (good and lines[n].rstrip() == "")
        if not good:
            print(f"{filename} invalid header (error on line: {n})")
        return good


all_good = True
directories_to_check = ["include", "src", "test"]

for d in directories_to_check:
    for path, __, names in os.walk(d):
        for n in names:
            if n.endswith(".h") or n.endswith(".cc"):
                full_name = os.path.join(path, n)
                if not check_file(n, full_name):
                    all_good = False


exit(0 if all_good else 1)
