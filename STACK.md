# Stack

Checked on 9 October 2026.

| Tool | Version used | Why | Source |
|---|---|---|---|
| C | C17 (`-std=c17`) | The user asked for pure C | ISO/IEC 9899:2018 |
| gcc | 13.3.0 | Compiler | https://gcc.gnu.org |
| libX11 | 1.8.7 (Ubuntu 24.04 build) | The only runtime dependency: one window, input, `XPutImage` | https://www.x.org/releases/current/doc/libX11/libX11/libX11.html |
| Linux `mount(2)` `MS_BIND` | kernel 6.18 here | The reboot-scoped block | https://man7.org/linux/man-pages/man2/mount.2.html |
| glibc `backtrace_symbols_fd` | 2.39 | Crash stack without allocating | https://man7.org/linux/man-pages/man3/backtrace.3.html |
| Font | X.Org font-adobe-75dpi helvR12, helvB12 | A period-correct bitmap face with a permissive licence | `NOTICE` |
| clang-format, clang-tidy | 18.1.3 | Format and lint | https://clang.llvm.org |
| cppcheck | 2.13.0 | Second linter | https://cppcheck.sourceforge.io |
| shellcheck | Ubuntu 24.04 package | Shell lint | https://www.shellcheck.net |
| lizard | 1.24.1 | Function length, parameters, complexity | https://github.com/terryyin/lizard |
| gcovr | 8.6 | Coverage gate | https://gcovr.com |
| AddressSanitizer, UBSan | gcc 13.3 | Memory and undefined-behaviour checks in tests | https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html |
| Xvfb, xdotool 3.20160805 | Ubuntu 24.04 packages | Real X server and real input for end-to-end tests | https://www.x.org |
| gitleaks | 8.30.0 | Secret scan | https://github.com/gitleaks/gitleaks |
| semgrep | 1.180.0, rules from github.com/semgrep/semgrep-rules `c/` | Static security rules | https://semgrep.dev |

## Advisories checked

- **libX11 CVE-2026-94285**: out-of-bounds read in the byte-oriented codeset
  parser, fixed upstream in 1.8.14; "could be used by malicious X servers to
  crash attached X clients" (SUSE advisory, moderate, CVSS 5.1). lockin
  talks to the user's own X server, so exposure is a crash at worst. Use a
  distro build with the fix.

## Facts the design rests on

- **Firefox keeps hosts-file names off DNS-over-HTTPS.** Its documentation
  lists "domains listed in /etc/hosts" among names never resolved through
  the trusted resolver (firefox-source-docs.mozilla.org, DNS over HTTPS).
- **Chromium honours the hosts file**: its built-in resolver reads HOSTS
  itself, and otherwise the system resolver does
  (chromium.googlesource.com, `net/dns/README.md`).
- **A running Chromium picks up the bind mount within a second.** Measured
  here on 9 October 2026 with Chromium 141 (Playwright build 1194): a page
  that loaded before the mount failed with `ERR_CONNECTION_REFUSED` 1 s
  after it. The sandbox runs gVisor, so repeat this on real hardware.
- **sudo keeps `DISPLAY` and `XAUTHORITY`** in its default environment list
  (`sudo -V` on Ubuntu 24.04), which is why `sudo lockin` can open a window.

## Not checked

- The block list's host names come from memory, apart from the Discord
  domains (techlockdown.com guide: discord.com, discordapp.com, discord.gg,
  discordapp.net). A wrong name blocks nothing and breaks nothing. GitHub's
  Copilot allowlist page could not be fetched.
- Flatpak and Snap browsers: whether a sandbox started before the lock sees
  the new hosts file was not tested.
