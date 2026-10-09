#ifndef LOCKIN_CORE_BLOCKLIST_H
#define LOCKIN_CORE_BLOCKLIST_H

#include "core/config.h"

/* First and last line of the section lockin adds to the hosts file. */
extern const char blocklist_marker[];
extern const char blocklist_end[];

/* True for a plain DNS host name: labels of letters, digits and hyphens
 * joined by dots, at least two labels, under CONFIG_HOST_MAX bytes. Anything
 * else is refused so a setting can never add a line of its own to the hosts
 * file. */
bool host_valid(const char *host);

/* Writes the hosts file to use while locked: an IPv4 and an IPv6 null route
 * for every built-in and configured host that is not in the allow list, then
 * `original` minus any earlier lockin section and minus lines that give a
 * blocked host an address of its own. Returns the number of
 * hosts blocked. */
int blocklist_render(const struct config *cfg, const char *original, struct sbuf *out);

#endif
