#!/bin/bash
# Security scans. Fails on any finding.
#
# semgrep needs its C rules on disk: semgrep.dev is often unreachable from
# build machines, so the rules are cloned from GitHub into build/ once.
set -euo pipefail
cd "$(dirname "$0")/.."

echo "gitleaks: secrets in the working tree"
gitleaks dir --no-banner --redact --config .gitleaks.toml .

rules=build/semgrep-rules
if [ ! -d "$rules/c" ]; then
    mkdir -p build
    git clone --quiet --depth 1 https://github.com/semgrep/semgrep-rules.git "$rules"
fi

echo "semgrep: C security rules"
# insecure-use-memset wants memset_s, which glibc does not have; every memset
# here zeroes a struct before use and none holds a secret.
semgrep scan --quiet --error --metrics=off --config "$rules/c" \
    --exclude-rule build.semgrep-rules.c.lang.security.insecure-use-memset \
    --exclude build --exclude src/ui/font_data.c src tests
echo "scans: ok"
