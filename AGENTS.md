# For the next engineer or agent

## Before you say "done"

```sh
make fmt-check lint size test   # no root
make test-e2e coverage scan     # asks for sudo
```

All must pass. `make check` runs the lot. Paste new numbers into `GATES.md`.

## Layout

- `src/core`: pure logic. No OS headers beyond libc, no drawing.
- `src/ui`: canvas, font, controls. Knows nothing about sessions.
- `src/platform/platform.h`: the window interface. `platform_x11.c` implements it.
- `src/sys/sys.h`: files, clock, blocking, crash trace. `sys_linux*.c` implement it.
- `src/app`: screens, actions, `store.c` (every file path), `main.c`.
- `tests/fake_sys.c`: the in-memory `sys.h` the unit tests link instead of Linux.

Dependencies point one way: `app` uses `ui`, `core` and the two interfaces;
`ui` uses `core`; `core` uses nothing.

## Rules

- C17, formatted by clang-format, warnings are errors. No `malloc`: storage
  is fixed-size and bounded, strings go through `struct sbuf`.
- Function 40 lines, 4 parameters, complexity 10; file 300 lines.
- Anything that can fail takes a `struct error *` and returns `bool`. The
  message says what failed and what to do. Log it once, in `app` or `main`.
- No globals in `core`, `ui` or `app`. `sys_linux*.c` and `platform_x11.c`
  keep one static struct each of process-wide facts; the crash handler keeps
  one descriptor because signal handlers get no context.
- A comment says why. A test name says what behaviour it proves.
- Every bug gets a failing test first.

## Never do

- Never write to `/etc/hosts` itself. The block is a bind mount of a file in
  `/run`, which is what makes it end at reboot and survive a crash safely.
- Never run file work as root. `sys_start` switches the effective user to
  the person behind sudo; only `sys_block` switches back, for the mount.
- Never put a setting's text into the hosts file without `host_valid`.
- Never make Lock in the default button or give the lock a keyboard shortcut.
- Never add a way to exit while locked to a release build. The `SIGUSR1`
  exit exists only under `LOCKIN_COVERAGE`.
- Never run `tests/system.sh` or `tests/e2e.sh` outside `unshare -m`.

## Recipe: add a screen

1. Add the value to `enum screen` in `src/app/app.h` and declare `screen_x`.
2. Write `src/app/screen_x.c`: draw with `ui_*`, call an `action_*` on a click.
3. Add it to the table in `app_frame` in `src/app/app.c`.
4. State changes go in `src/app/actions.c`, never in the drawing code.
5. Tests: behaviour in `tests/test_app*.c` through `driver_click`, the
   picture in `tests/test_screens.c`, one pass in `tests/e2e.sh`.

## Recipe: port to another OS

Write `src/platform/platform_<os>.c` (four functions) and `src/sys/sys_<os>*.c`
(the functions in `sys.h`), and pick them in the Makefile's `LINUX` list.
`core`, `ui` and `app` do not change. See `docs/adr/0002`.

## Generated files

`src/ui/font_data.c` comes from `tools/genfont.py`; regenerate with `make docs`.
