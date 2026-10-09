# 0001: Block sites with a bind mount over /etc/hosts

## Context
The user wants sites blocked from one click until shutdown. The block must
end by itself at reboot and must not be able to leave the machine broken.

## Decision
Write the blocked hosts file to `/run/lockin/hosts` and bind-mount it over
`/etc/hosts`.

## Consequences
- Reboot removes it: `/run` is a tmpfs and mounts are kernel state. No
  unlock code, no boot-time service.
- The file on disk is never modified, so a crash or power cut is harmless.
- Root can remove it with `umount /etc/hosts`.
- Programs in a private mount namespace started before the lock (some
  Flatpak and Snap apps) may keep the old file until restarted.
- Hosts files have no wildcards; every host name is listed.

## Alternatives rejected
- **Edit `/etc/hosts` in place.** Survives reboot, so it needs a boot-time
  clean-up unit; if that is missing or fails, the user is locked out for good.
- **nftables rules by IP.** CDN addresses change by the minute.
- **A local DNS proxy.** A daemon and a resolver change, for the same result.
