#!/bin/bash
# Runs tests/linux_tests.c scenario by scenario, each on the stage it needs.
# Must run as root inside its own mount namespace:
#
#   sudo unshare -m ./tests/system.sh build/asan
#
# Nothing here reaches the real /etc/hosts or /run: both are private to the
# namespace and vanish when this script exits.
set -euo pipefail

build=${1:?usage: system.sh BUILD_DIR}
bin=$(realpath "$build")/linux_tests

[ "$(id -u)" = 0 ] || { echo "system.sh needs root. Run: sudo unshare -m $0 $build" >&2; exit 1; }

# A real, unprivileged account to stand in for "the user behind sudo".
user=${SUDO_USER:-$(getent passwd 1000 | cut -d: -f1)}
user=${user:-nobody}
uid=$(id -u "$user")
gid=$(id -g "$user")

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
chmod 755 "$work"
# Coverage builds write their counts next to the objects, also when a
# scenario ends as the unprivileged user.
chmod -R a+rwX "$build"
umask 000
mount -t tmpfs tmpfs /run

stage() {
    rm -rf "${work:?}/home"
    mkdir -p "$work/home"
    chown "$1" "$work/home"
    export XDG_CONFIG_HOME=$work/home/config XDG_STATE_HOME=$work/home/state XDG_DATA_HOME=$work/home/data
}

run() {
    echo "== $*"
    "$@"
}

as_user() {
    setpriv --reuid "$uid" --regid "$gid" --clear-groups "$@"
}

owner=${SUDO_UID:-}
unset SUDO_UID SUDO_USER SUDO_GID

stage root
run "$bin" clock
run "$bin" files
run "$bin" tail
run "$bin" file-errors
run "$bin" crash "$work/crash.log"
run "$bin" refuse
SUDO_UID=abc run "$bin" bad-uid
SUDO_UID='' run "$bin" bad-uid
SUDO_UID=54321 run "$bin" no-user
XDG_CONFIG_HOME=/proc/no-such-place run "$bin" bad-dir
XDG_STATE_HOME=/$(printf 'x%.0s' {1..300}) run "$bin" bad-dir

stage "$uid:$gid"
run as_user "$bin" unprivileged
run as_user "$bin" files

# Blocking, each in a namespace of its own so one cannot see another's mount.
stage root
run unshare -m "$bin" block

# LeakSanitizer stops the process with ptrace to scan it, which the kernel
# forbids once real and effective users differ. Nothing here allocates, and
# the unit tests keep leak checking on.
export ASAN_OPTIONS=detect_leaks=0

stage "$uid:$gid"
cp /bin/sleep "$work/lk-victim"
chmod 755 "$work/lk-victim"
SUDO_UID=$uid run unshare -m "$bin" sudo "$work/lk-victim"

# No hosts file at all: /etc is replaced by a copy without one.
stage root
mkdir "$work/etc"
cp /etc/passwd /etc/group /etc/nsswitch.conf "$work/etc/"
run unshare -m sh -c "mount --bind '$work/etc' /etc && exec '$bin' block-fails"

# Hand the build tree back to whoever ran sudo.
if [ -n "$owner" ]; then
    chown -R "$owner" "$build"
fi
echo "system tests passed"
