#!/bin/bash
# End-to-end tests: the real program, a real X server (Xvfb), real clicks and
# key presses (xdotool), the real hosts-file block. Must run as root inside
# its own mount namespace:
#
#   sudo unshare -m ./tests/e2e.sh build/asan
#
# The block lands on this namespace's /etc/hosts only and is gone on exit.
set -euo pipefail

build=$(realpath "${1:?usage: e2e.sh BUILD_DIR}")
app=$build/lockin
[ "$(id -u)" = 0 ] || { echo "e2e.sh needs root. Run: sudo unshare -m $0 $1" >&2; exit 1; }

user=${SUDO_USER:-$(getent passwd 1000 | cut -d: -f1)}
user=${user:-nobody}
uid=$(id -u "$user")
gid=$(id -g "$user")
owner=${SUDO_UID:-}
unset SUDO_UID SUDO_USER SUDO_GID
# See tests/system.sh: leak scanning cannot run once users are switched.
export ASAN_OPTIONS=detect_leaks=0

work=$(mktemp -d)
chmod 755 "$work"
chmod -R a+rwX "$build"
umask 000
mount -t tmpfs tmpfs /run
pids=()
cleanup() {
    kill -9 "${pids[@]}" 2>/dev/null || true
    rm -rf "$work"
    if [ -n "$owner" ]; then chown -R "$owner" "$build"; fi
}
trap cleanup EXIT

fail() {
    echo "FAIL: $*" >&2
    if [ -f "$work/stderr" ]; then sed 's/^/  stderr: /' "$work/stderr" >&2; fi
    exit 1
}

# Each scenario gets empty settings, state and data folders owned by $1.
stage() {
    rm -rf "${work:?}/home" "${work:?}/proj"
    mkdir -p "$work/home" "$work/proj"
    chown "$1" "$work/home" "$work/proj"
    export XDG_CONFIG_HOME=$work/home/config XDG_STATE_HOME=$work/home/state XDG_DATA_HOME=$work/home/data
}

csv() { cat "$XDG_DATA_HOME/lockin/sessions.csv" 2>/dev/null || true; }
log() { cat "$XDG_STATE_HOME/lockin/lockin.log" 2>/dev/null || true; }
resolved() { getent ahostsv4 "$1" 2>/dev/null | awk 'NR==1 {print $1}'; }

start_x() { # display, screen spec
    Xvfb "$1" -screen 0 "$2" -ac -nolisten tcp >/dev/null 2>&1 &
    pids+=($!)
    for _ in $(seq 50); do
        if DISPLAY=$1 xdotool getdisplaygeometry >/dev/null 2>&1; then return; fi
        sleep 0.1
    done
    fail "Xvfb $1 did not start"
}

start_app() { # command prefix, e.g. setpriv ... or env
    window=
    "$@" "$app" 2>"$work/stderr" &
    app_pid=$!
    pids+=("$app_pid")
    for _ in $(seq 50); do
        window=$(xdotool search --name '^Lock in$' 2>/dev/null | head -1 || true)
        if [ -n "$window" ]; then break; fi
        sleep 0.1
    done
    [ -n "$window" ] || fail "the window did not open"
    sleep 0.3
}

alive() { kill -0 "$app_pid" 2>/dev/null; }

# Waits for the app to exit by itself and returns its exit status.
wait_exit() {
    local status=0
    for _ in $(seq 40); do
        if ! alive; then
            wait "$app_pid" || status=$?
            return "$status"
        fi
        sleep 0.1
    done
    fail "the program is still running"
}

# Ends a locked app. Coverage builds leave on SIGUSR1 so their counts are
# written; other builds have no way out but SIGKILL, which is the point.
stop_app() {
    kill -USR1 "$app_pid" 2>/dev/null || true
    sleep 0.8
    kill -9 "$app_pid" 2>/dev/null || true
    wait "$app_pid" 2>/dev/null || true
}

click() {
    xdotool mousemove "$1" "$2" click 1
    sleep 0.15
}
keys() {
    xdotool key --delay 20 "$@"
    sleep 0.1
}
typed() {
    xdotool type --delay 5 "$1"
    sleep 0.1
}
as_user() { setpriv --reuid "$uid" --regid "$gid" --clear-groups "$@"; }
close_window() {
    "$build/x11_send" "$window"
    sleep 0.4
}

scenario() { echo "== $1"; }

scenario "command line: version, help, bad options, settings check"
stage root
"$app" --version | grep -q '^lockin 1\.' || fail "--version"
"$app" --help | grep -q 'sudo lockin' || fail "--help"
status=0
"$app" --bogus 2>"$work/stderr" || status=$?
[ "$status" = 2 ] || fail "a bad option must exit 2"
grep -q 'lockin --check' "$work/stderr" || fail "a bad option must print usage"
status=0
"$app" --check extra 2>/dev/null || status=$?
[ "$status" = 2 ] || fail "extra arguments must exit 2"
"$app" --check 2>/dev/null | grep -q 'hosts would be blocked' || fail "--check on a fresh install"
"$app" --check 2>/dev/null | grep -q '^0.0.0.0 music.youtube.com$' || fail "--check lists the hosts"
printf 'slot_am = 25:99\nvolume = 11\n' >"$XDG_CONFIG_HOME/lockin/lockin.conf"
status=0
"$app" --check >/dev/null 2>"$work/stderr" || status=$?
[ "$status" = 2 ] || fail "bad settings must exit 2, got $status"
grep -q 'line 1: slot_am' "$work/stderr" || fail "line 1 problem not listed"
grep -q 'line 2: unknown setting "volume"' "$work/stderr" || fail "line 2 problem not listed"

scenario "no display: exits 1 and logs how to fix it"
stage root
status=0
DISPLAY=:55 "$app" 2>"$work/stderr" || status=$?
[ "$status" = 1 ] || fail "expected exit 1, got $status"
log | grep -q 'ERROR E_DISPLAY Cannot open the X display ":55"' || fail "E_DISPLAY not logged"
status=0
env -u DISPLAY "$app" 2>"$work/stderr" || status=$?
log | grep -q 'DISPLAY is not set' || fail "unset DISPLAY not explained"

scenario "a folder that cannot be created: exits 1 before any window"
status=0
XDG_STATE_HOME=/proc/nope "$app" 2>"$work/stderr" || status=$?
[ "$status" = 1 ] || fail "expected exit 1, got $status"
grep -q 'Cannot create the folder' "$work/stderr" || fail "no reason given"

scenario "8-bit display: refused with a reason"
start_x :96 640x480x8
stage root
status=0
DISPLAY=:96 "$app" 2>"$work/stderr" || status=$?
[ "$status" = 1 ] || fail "expected exit 1, got $status"
log | grep -q 'not 24-bit true colour' || fail "the colour depth error is not logged"

scenario "tall screens: the window picks 2x at 1800 lines and 3x at 2800"
start_x :94 1200x1800x24
start_x :95 1800x2800x24
stage root
export DISPLAY=:94
start_app env
xdotool getwindowgeometry "$window" | grep -q 'Geometry: 1160x1000' || fail "expected 2x on an 1800-line screen"
close_window
wait_exit || fail "closing should exit 0"
export DISPLAY=:95
start_app env
xdotool getwindowgeometry "$window" | grep -q 'Geometry: 1740x1500' || fail "expected 3x on a 2800-line screen"
close_window
wait_exit || fail "closing should exit 0"

start_x :97 1280x1024x24
export DISPLAY=:97

scenario "ordinary user: window opens, Lock in is disabled, window closes"
stage "$uid:$gid"
start_app as_user
click 518 452
[ "$(resolved music.youtube.com)" != 0.0.0.0 ] || fail "an ordinary user managed to block sites"
alive || fail "the program died"
close_window
wait_exit || fail "closing the window should exit 0"
log | grep -q 'INFO started lockin' || fail "no start line in the log"

scenario "crash: a fatal signal leaves a stack trace in lockin.log"
stage root
start_app env
kill -SEGV "$app_pid"
status=0
wait_exit || status=$?
[ "$status" = 139 ] || fail "expected death by SIGSEGV (139), got $status"
log | grep -q 'lockin crashed on signal 11' || fail "no crash report in the log"
log | grep -q 'addr2line' || fail "no addr2line hint in the log"

scenario "missed slot: logged, then the Close button exits"
stage root
start_app env
click 150 452
csv | grep -q ',missed,' || fail "the missed slot is not in the log"
click 508 452
wait_exit || fail "Close should exit 0"

scenario "scale = 2: the window doubles and clicks still land"
stage root
mkdir -p "$XDG_CONFIG_HOME/lockin"
echo 'scale = 2' >"$XDG_CONFIG_HOME/lockin/lockin.conf"
start_app env
xdotool getwindowgeometry "$window" | grep -q 'Geometry: 1160x1000' || fail "the window is not 1160x1000"
click 300 904
csv | grep -q ',missed,' || fail "a click at doubled coordinates missed the button"
click 1016 904
wait_exit || fail "Close should exit 0"

scenario "full session under sudo: block, close programs, refuse to quit, log"
stage "$uid:$gid"
as_user mkdir -p "$XDG_CONFIG_HOME/lockin"
as_user sh -c "echo 'project = $work/proj' >'$XDG_CONFIG_HOME/lockin/lockin.conf'"
cp /bin/sleep "$work/Discord"
chmod 755 "$work/Discord"
as_user "$work/Discord" 300 &
victim=$!
pids+=("$victim")
SUDO_UID=$uid start_app env
click 300 92
typed "Finish kilo chapter 3x"
keys BackSpace Tab
typed "Rows render"
keys Tab shift+Tab ISO_Left_Tab Tab Tab
typed "Discord"
keys Return KP_Enter F5
[ "$(resolved music.youtube.com)" != 0.0.0.0 ] || fail "Enter in a text box locked the machine"
xdotool mousemove 518 452 click 3
[ "$(resolved music.youtube.com)" != 0.0.0.0 ] || fail "a right click pressed Lock in"
click 518 452
[ "$(resolved music.youtube.com)" = 0.0.0.0 ] || fail "music.youtube.com is not blocked"
[ "$(resolved discord.com)" = 0.0.0.0 ] || fail "discord.com is not blocked"
[ "$(resolved api.anthropic.com)" = 0.0.0.0 ] || fail "api.anthropic.com is not blocked"
grep -q '^127.0.0.1 localhost' /etc/hosts || fail "the original hosts entries are gone"
sleep 2.5
if kill -0 "$victim" 2>/dev/null; then fail "the Discord process was not closed"; fi
[ "$(stat -c %u "$XDG_STATE_HOME/lockin/state")" = "$uid" ] || fail "state file is not owned by the user"
grep -q '^wish = Finish kilo chapter 3$' "$XDG_STATE_HOME/lockin/state" || fail "WOOP was not saved"
close_window
alive || fail "the locked window closed"
kill -TERM "$app_pid"
kill -INT "$app_pid"
kill -HUP "$app_pid"
sleep 0.3
alive || fail "the locked program stopped on a polite signal"
click 350 100
click 508 452
keys Escape
click 508 452
click 518 452
if csv | grep -q ',done,'; then fail "an empty form was saved"; fi
click 300 74
typed "src/kilo.c:212 write editorScroll"
click 300 100
typed "rows draw, 1 commit"
click 276 257
click 518 452
csv | grep -q ',done,.*,1,3,silence,.*"rows draw, 1 commit"' || fail "the session row is wrong: $(csv)"
[ "$(cat "$work/proj/NEXT.md")" = "src/kilo.c:212 write editorScroll" ] || fail "NEXT.md is wrong"
[ "$(stat -c %u "$work/proj/NEXT.md")" = "$uid" ] || fail "NEXT.md is not owned by the user"
click 50 452
click 518 452
click 508 452
alive || fail "the program died after the session"
stop_app
[ "$(resolved music.youtube.com)" = 0.0.0.0 ] || fail "the block ended when the program did"

scenario "restart while blocked: it knows, and the window still cannot close"
SUDO_UID=$uid start_app env
close_window
alive || fail "a restarted, already-blocked window closed"
stop_app

echo "end-to-end tests passed"
