# Performance budget

There are no routes or queries. The budget is what a small always-open
desktop window should cost. Measured on 9 October 2026, release build, in
the build sandbox (gVisor), under Xvfb.

| Budget | Gate | Measured |
|---|---|---|
| Start to window on screen | <= 100 ms | 11 ms (includes the test's polling) |
| Idle CPU, start screen | <= 1% of one core | 0.4% (8 of 2000 ticks over 20 s) |
| Resident memory | <= 16 MB | 5.8 MB |
| Binary, stripped | <= 200 KB | 92 KB (341 KB with symbols and debug info) |
| Wake-ups when idle | 2 per second | 2 (500 ms tick) |
| Program sweep while locked | every 2 s | 2 s (`APP_SWEEP_SECONDS`) |

The window redraws fully on every event and tick. That is deliberate: at
580x500 it is cheaper than tracking damage, and it is what the 0.4% buys.

Not measured: time per frame in isolation, and behaviour on a real GPU-backed
X server. The numbers above come from one machine.
