# Decisions

## Answered by the user (9 October 2026)

| Question | Answer |
|---|---|
| How long does the lock last? | Until reboot |
| What is blocked? | Music sites, AI chat sites, AI APIs, Discord |
| Which display system first? | X11 (XWayland covers Wayland desktops) |
| How hard does it resist cheating? | Honest friction: no watchdog; root can undo it by hand |
| Look | Windows 2000 style |
| Scope | Everything in the focus research protocol; proper error handling and crash reports |

## Taken without asking

The design was not reviewed separately; the user said to continue. Each of
these is cheap to change.

1. **No Docker.** A desktop program that mounts over `/etc/hosts` and opens
   an X window is not a container workload. See `docs/adr/0003`.
2. **Enter never locks.** Lock in is not the default button, because the
   action cannot be undone before a reboot.
3. **Text boxes edit at the end only** (type and Backspace). It keeps the
   toolkit small; the fields are one line each.
4. **ASCII text.** Other characters draw as `?` and cannot be typed.
5. **Programs are closed with SIGKILL**, every 2 seconds while locked, and
   only if they belong to the user who ran sudo.
6. **The block also removes existing hosts lines** that give a blocked host
   an address, because the resolver would otherwise hand that address out
   too. The real file on disk is untouched.
7. **The coin is derived, not rolled**: a hash of a per-install seed, the
   date and the slot. Reopening the program cannot change it.
8. **A verdict needs 14 sessions, 5 per condition, and a 20% margin**, from
   the research report's decision rule.
9. **Sessions are measured from Lock in to Save.** The report's Neovim
   autocmd (first-keystroke time) is editor configuration and is out of scope.
10. **Daylight saving is not modelled.** Sri Lanka has none.
11. **Branch coverage counts branches the compiler can reach**
    (`--exclude-unreachable-branches`).

## Out of scope for 1.0

Native Wayland, Windows and macOS back ends; a watchdog; scheduling itself at
slot times; the Neovim focus profile and logging autocmd; a screen reader
interface (the window is a drawn bitmap).
