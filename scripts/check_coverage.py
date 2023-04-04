#!/usr/bin/env python3

import sys
import re

line_threshold = 100.0

print(f"Line coverage threshold {line_threshold}%")

summary = open(sys.argv[1]).read().split('\n')
lines = summary[2]
pl = float(re.findall("[0-9]?[0-9][0-9].[0-9]%", lines)[0][:-1])

print(f"coverage: {pl}%")

coverage_met = pl >= line_threshold

if not coverage_met:
    print("Coverage not met :(")

exit(0 if coverage_met else 1)
