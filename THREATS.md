# Threat model

lockin is a local, single-user program with no network code. The asset is
root: it is started with sudo, so a bug in it is a path from "runs as the
user" to "runs as root". The OWASP Top 10 lists are about web services; the
items that have a meaning here (injection, broken access control, security
misconfiguration, logging failures) are covered by the rows below, and the
rest (authentication, SSRF, cryptography, sessions) have no surface.

| # | Threat (STRIDE) | Mitigation | Where | Test |
|---|---|---|---|---|
| 1 | **Elevation**: a setting injects a line into the hosts file (`block = a.com\n1.2.3.4 bank.com`) | Hosts must match a strict DNS-name grammar; settings are single lines | `host_valid`, `kv_line` | `a_host_must_be_a_plain_dns_name` |
| 2 | **Elevation / tampering**: root follows a symlink the user planted (`NEXT.md -> /etc/shadow`) | All user-file work runs with the effective uid of the invoking user; opens use `O_NOFOLLOW` | `sys_start`, `write_file` | system `sudo` (files owned by the user, euid checked), `file-errors` (symlink refused) |
| 3 | **Elevation**: memory corruption in a parser while root is recoverable | No heap; every buffer bounded through `sbuf`; parsers reject oversize input; ASan and UBSan on every test | `core/text.c`, parsers | whole suite under sanitizers; `refuses_values_that_are_too_long`, `damaged_rows_are_rejected`, `an_oversized_hosts_file_is_refused_not_truncated` |
| 4 | **Denial of service**: the kill list is used to kill other users' or root's processes | Only processes whose owner is the invoking user | `process_name` | system `sudo` (root's same-named process survives) |
| 5 | **Tampering / DoS**: a crash or power cut leaves the machine's hosts file broken | The real file is never written; the block is a bind mount | `sys_block` | system `block`, `block-fails`; e2e checks the original entries survive |
| 6 | **Tampering**: a log field is run as a formula when the CSV is opened in a spreadsheet | Fields starting with `= + - @` get a leading apostrophe | `csv_put` | `csv_defuses_spreadsheet_formulas` |
| 7 | **Repudiation / information**: failures vanish, or the log leaks private text | Every error is logged once with a stable code; the log holds no WOOP text or form contents | `store_log_error` | `a_failed_block_stays_put_and_explains`, `log_lines_carry_time_and_level` |
| 8 | **Spoofing**: `SUDO_UID` is forged | It is only read when the process already is root; a root caller can do anything anyway. Malformed values are ignored, unknown uids refused | `invoking_user` | system `bad-uid`, `no-user` |
| 9 | **DoS**: a hostile X server crashes the client (libX11 CVE-2026-94285) | The server is the user's own; impact is a crash; keep libX11 patched | `STACK.md` | not testable here |

## Accepted risks

- **Root can undo the lock** (`umount /etc/hosts`, `kill -9`). Chosen by the
  user: "honest friction". The block list is friction, not a security control.
- **Saved root.** Between Lock in and exit the process could regain root. A
  full drop would let the user kill the window without sudo, which defeats
  the purpose. Rows 2 and 3 are what keep that capability out of reach.
- **Hosts-file blocking is bypassable** by a proxy, a VPN, DNS inside a
  sandboxed browser, or a tab that is already open.
