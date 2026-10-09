# Gates

Last run: 9 October 2026, Ubuntu 24.04 (gcc 13.3, clang 18.1.3), as root in
the build sandbox (gVisor). `make check` ran every line below and exited 0
in 168 s.

| Gate | Command | Result |
|---|---|---|
| Format | `make fmt-check` | clean |
| Compiler warnings | `-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror` and 6 more | 0 |
| clang-tidy (bugprone, cert, analyzer, misc, performance, portability, readability) | `make lint` | 0 findings |
| cppcheck (warning, style, performance, portability) | `make lint` | 0 findings |
| shellcheck | `make lint` | 0 findings |
| Function <= 40 lines, <= 4 parameters, complexity <= 10; file <= 300 lines | `make size` | `size limits: ok` |
| Unit tests, ASan + UBSan, three runs in a row | `make test` | `109 tests, 0 failed` x 3 |
| System tests on real Linux (mount, users, signals, crash) | `sudo unshare -m tests/system.sh` | 16 scenario runs, all `0 failed`; `system tests passed` |
| End-to-end, real X server and real input | `sudo unshare -m tests/e2e.sh` | 11 scenarios; `end-to-end tests passed` |
| Line coverage >= 95% | `make coverage` | **98.2%** (1780 of 1813) |
| Branch coverage >= 95% | `make coverage` | **95.4%** (1047 of 1098) |
| Function coverage | `make coverage` | 98.6% (218 of 221) |
| Secrets | `gitleaks dir` 8.30.0 | `no leaks found` |
| Static security | `semgrep` 1.180.0, 15 C rules, 51 files | 0 findings |
| README quick start in a fresh clone | `git clone`, `make build`, `lockin --version`, `lockin --check` | works as written |
| Performance | see `PERF.md` | all within budget |

## Suppressions

Two, both for one clang-analyzer false positive
(`valist.Uninitialized` on a `vsnprintf` directly after `va_start`), in
`src/core/err.c` and `src/core/text.c`. Eight clang-tidy checks are off
project-wide, each with its reason in `.clang-tidy`. One semgrep rule is off
(`insecure-use-memset` asks for `memset_s`, which glibc lacks).

## Coverage exclusions

`src/ui/font_data.c` (generated data, no logic). Nothing else.
The uncovered 4.6% of branches are system-call failures that cannot be made
to happen from a test without fault injection (`fstat`, `close`, `opendir`,
`getrandom`, `setegid` and the like in `src/sys`) and X11 visual checks.

## Bugs the tests found before you did

1. The CSV header was written before every row once the log passed 7 bytes.
2. A window left on the start screen overnight kept yesterday's slot.
3. A host that already had an address in `/etc/hosts` stayed reachable,
   because the resolver returns every address listed for a name.
4. `rm -rf "$work/home"` in the test scripts had no guard against an empty
   `$work` (shellcheck SC2115).

## Not run

- On real hardware, on a real desktop, under a real `sudo`. Everything ran
  in a sandbox with Xvfb and no window manager; `SUDO_UID` was set by hand.
- Native Wayland without XWayland (not supported).
- Firefox, and Flatpak or Snap browsers. Chromium 141 was checked once by
  hand: a running browser lost access 1 s after the mount.
- A reboot. That the block ends at reboot follows from `/run` being a tmpfs
  and from mounts not surviving one; it was not observed.
- valgrind (the sanitizers cover the same ground for this code).
