#!/bin/bash
# Size limits for hand-written code. Fails listing every offender.
#   function: 40 lines, 4 parameters, cyclomatic complexity 10
#   file:     300 lines
set -euo pipefail
cd "$(dirname "$0")/.."

status=0
lizard --CCN 10 --length 40 --arguments 4 --warnings_only \
    --exclude 'src/ui/font_data.c' src tests || status=1

while IFS= read -r file; do
    lines=$(wc -l <"$file")
    if [ "$lines" -gt 300 ]; then
        echo "$file: $lines lines, limit 300"
        status=1
    fi
done < <(find src tests -name '*.[ch]' ! -name font_data.c)

if [ "$status" = 0 ]; then echo "size limits: ok"; fi
exit "$status"
