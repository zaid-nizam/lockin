# Architecture

## Shape

One process, one window, no network, no threads, no heap.

```
              +---------------- src/app ----------------+
              | main.c   screens   actions   store.c    |
              +----+-----------+-----------------+------+
                   |           |                 |
          src/ui (toolkit)   src/core      interfaces only
          canvas, font,      slots, rows,  platform.h   sys.h
          controls           stats, config,     |          |
                |            block list     platform_   sys_linux*.c
                +--> core                   x11.c       (files, clock,
                                            (window)     mount, kill,
                                                         crash trace)
```

- `core` is pure: given the same inputs it returns the same outputs. It is
  where every rule from the research lives, and it is tested without an OS.
- `ui` draws into a `uint32_t` array. It has never heard of X11.
- `platform.h` is four functions: open, wait for input, show pixels, close.
- `sys.h` is everything else the OS provides. Unit tests link
  `tests/fake_sys.c` in its place, which is why the seam exists.

The Makefile chooses the implementation files; there are no function-pointer
tables. A port is new files, not edits (ADR 0002).

## Trust boundaries

| Boundary | What crosses it | Guard |
|---|---|---|
| User's files to a process that can become root | `lockin.conf`, `state`, `sessions.csv`, `NEXT.md` | Read with the effective user set to the invoking user; bounded parsers; `host_valid` before anything reaches the hosts file |
| The process to the system hosts file | The rendered block | Root only inside `sys_block`; written to `/run/lockin/hosts`, bind-mounted; the real file is never opened for writing |
| The process to other processes | `SIGKILL` | Only processes owned by the invoking user, matched by exact name |
| X server to the process | Events | Fixed-size event translation; unknown events dropped |

## Failure behaviour

| Dependency | If it fails | Mode |
|---|---|---|
| X display | Exit 1, reason in the log and on stderr | fail closed |
| Settings file | Exit 2, every problem listed | fail closed |
| Mount | Stay on the start screen, reason in the status bar, nothing changed | fail closed |
| State file write | Logged; the lock or save goes ahead | fail open (convenience data) |
| Session log or NEXT.md write | Form stays open with the reason; Save can be pressed again | fail closed |
| Unreadable state, log or NEXT.md | Logged, treated as empty | fail open |
| Error log write | stderr only | fail open |

No call blocks on anything but the local filesystem and the X socket, so
there are no timeouts to set.

## The three flows that matter

**Lock in** (`action_lock`): read the current hosts file -> render the block
(pure) -> `sys_block`: become root, write `/run/lockin/hosts`, bind-mount it
over `/etc/hosts` unless already mounted, drop root -> save WOOP answers ->
ignore SIGINT/SIGTERM/SIGHUP/SIGQUIT -> close listed programs -> start the
session clock. If any step up to and including the mount fails, nothing has
changed and the user is still on the start screen.

**Save** (`action_save`): build the row from the session -> check the form,
collecting every problem -> write `NEXT.md` (atomic replace) -> append the
row -> remember sleep times -> Done screen. `NEXT.md` goes first because
writing it twice is harmless and the row must not be written twice.

**Tick** (`app_tick`, twice a second): refresh the clock; on the start screen
re-pick the slot; every 2 s while blocked, close listed programs.

## Why the block ends at reboot

`/run` is a tmpfs and bind mounts are kernel state. A reboot discards both.
There is no unlock code to get wrong, and a crash or power cut cannot leave
`/etc/hosts` edited, because it never is (ADR 0001).

## Privileges

Started under sudo, the process keeps real uid 0 and sets its effective uid
to the invoking user. Files it creates are the user's. Only root can signal
it, which is the "honest friction" the user chose. It raises to root in one
function, for one `mkdir`, one `open` and one `mount`.
