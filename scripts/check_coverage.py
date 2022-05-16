#!/usr/bin/env python3

import sys
import re

line_threshold = 95.0
function_threshold = 80.0

print(f"Coverage threshold: lines={line_threshold}%, functions={function_threshold}%")

summary = open(sys.argv[1]).read().split('\n')
lines = summary[2]
functions = summary[3]
pl = float(re.findall("[0-9]?[0-9][0-9].[0-9]%", lines)[0][:-1])
pf = float(re.findall("[0-9]?[0-9][0-9].[0-9]%", functions)[0][:-1])

print(f"line coverage: {pl}%")
print(f"function coverage: {pf}%")

coverage_met = pl > line_threshold and pf > function_threshold

if not coverage_met:
    print("Coverage not met :(")

exit(0 if coverage_met else 1)
