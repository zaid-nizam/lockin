#!/bin/bash
# Checks every tool the build and the tests need, and prints the fix for
# each one that is missing. Exits non-zero if anything required is absent.
set -uo pipefail

missing=0
need() { # command, purpose, how to install
    if command -v "$1" >/dev/null 2>&1; then
        printf '  ok       %-13s %s\n' "$1" "$2"
    else
        printf '  MISSING  %-13s %s\n           fix: %s\n' "$1" "$2" "$3"
        missing=1
    fi
}

echo "To build and run:"
need cc "C17 compiler (gcc 11+ or clang 14+)" "sudo apt install build-essential"
need make "GNU make" "sudo apt install make"
if [ -f /usr/include/X11/Xlib.h ]; then
    echo "  ok       libx11-dev    X11 headers"
else
    echo "  MISSING  libx11-dev    X11 headers"
    echo "           fix: sudo apt install libx11-dev"
    missing=1
fi

echo "To run the tests (make check):"
need Xvfb "virtual X server for end-to-end tests" "sudo apt install xvfb"
need xdotool "clicks and key presses for end-to-end tests" "sudo apt install xdotool"
need unshare "private mount namespace for tests" "sudo apt install util-linux"
need setpriv "running a test as an ordinary user" "sudo apt install util-linux"
need clang-format "formatter" "sudo apt install clang-format"
need clang-tidy "linter" "sudo apt install clang-tidy"
need cppcheck "linter" "sudo apt install cppcheck"
need shellcheck "shell linter" "sudo apt install shellcheck"
need gcovr "coverage report" "pip install gcovr"
need lizard "function size and complexity" "pip install lizard"
need gitleaks "secret scan" "https://github.com/gitleaks/gitleaks/releases"
need semgrep "static security scan" "pip install semgrep"

if [ "$missing" = 0 ]; then
    echo "Everything is here."
else
    echo "Install what is marked MISSING, then run make doctor again."
fi
exit "$missing"
