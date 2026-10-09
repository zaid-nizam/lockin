# Plan

**Goal:** a pure-C Linux desktop program that blocks distractions until
shutdown and runs the focus protocol from the user's research report.

**Status:** 1.0.0 built and verified on 9 October 2026. See `GATES.md`.

Requirements, in the user's words and where each landed:

| Asked for | Where |
|---|---|
| Pure C, own UI library, portable later | `src/ui`, `platform.h`, `sys.h`, ADR 0002 |
| Start with sudo, one click locks | `action_lock`, `sys_block` |
| Cannot open music, AI sites, OpenRouter, Discord | `src/core/blocklist.c` |
| Only closes at shutdown | `action_close`, `sys_refuse_to_stop`, ADR 0001 |
| Everything in the research report | WOOP, if-then card, 10-minute commitment, urge counter, hard stop, NEXT.md, session log, coin-flip experiment, 20% decision rule, month targets, the missed-slot script |
| 2000s UI, no AI slop | Windows 2000 colours, bevels and bitmap font |
| Proper error handling, debuggable crashes | `struct error`, `lockin.log`, `sys_trace_crashes` |

Next, if wanted: native Wayland back end; Windows and macOS ports; a systemd
user timer that opens the window at slot times.
