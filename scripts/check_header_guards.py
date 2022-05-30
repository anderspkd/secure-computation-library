#!/usr/bin/env python3

import os

def check_guards(path, expected_header_guard):
    with open(path, 'r') as f:
        opening_header = False
        lines = f.readlines()
        i = 0
        while i < len(lines) - 1:
            if lines[i].rstrip() == '#ifndef ' + expected_header_guard:
                if lines[i + 1].rstrip() == '#define ' + expected_header_guard:
                    opening_header = True
                break
            i += 1
        else:
            print('No opening header in', path)
            return False

        good = opening_header \
            and lines[-1].rstrip() == '#endif  // ' + expected_header_guard

        if not good:
            print(f'{path} invalid header')
            print('Expected:')
            print('#ifndef ' + expected_header_guard)
            print('#define ' + expected_header_guard)
            print('#endif  // ' + expected_header_guard)
            print('Found:')
            print(lines[i].rstrip())
            print(lines[i + 1].rstrip())
            print(lines[-1].rstrip())
            print('----')

        return good


to_truncate = len('include') + 1
all_good = True
for path, _, names in os.walk('include'):
    for n in names:
        truncated_path = path[to_truncate:]
        full_name = os.path.join(truncated_path, n)
        full_name = full_name.replace('/', '_').replace('.', '_')
        full_name = '_' + full_name
        full_name = full_name.upper()

        # check that there's an include guard with full_name as the symbol.

        if not check_guards(os.path.join(path, n), full_name):
            all_good = False

exit(0 if all_good else 1)
