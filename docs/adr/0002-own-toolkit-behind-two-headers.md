# 0002: A hand-written toolkit behind platform.h and sys.h

## Context
The user asked for pure C, a UI library of their own, a Windows 2000 look,
and a structure that makes Windows and macOS easy to add. The default would
be to adopt GTK or SDL.

## Decision
Draw every pixel in software into a plain array (`src/ui`), with an embedded
bitmap font. The platform layer only opens a window, delivers input and
copies the array to the screen. OS services sit behind `sys.h`.

## Consequences
- One runtime dependency (libX11). The look is identical everywhere.
- A port is `platform_<os>.c` (four functions) plus `sys_<os>*.c`.
- Pixels can be asserted in unit tests without a display.
- No accessibility tree, no text selection, ASCII only, no IME. Those are
  the price, and they are listed in `DECISIONS.md`.

## Alternatives rejected
- **GTK / Qt.** Not "pure C with my own UI library", and no 2000s look.
- **SDL.** Would replace about 200 lines of X11 code with a dependency.
- **X11 core fonts and drawing calls.** Ties the toolkit to X11.
