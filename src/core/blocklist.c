#include "core/blocklist.h"

#include <ctype.h>
#include <string.h>

const char blocklist_marker[] = "# lockin: blocked until the next reboot\n";
const char blocklist_end[] = "# lockin: end of block\n";

/* Hosts files have no wildcards, so each host a site is reached by is listed.
 * Desktop apps that use many hosts (Spotify, Discord) are closed by process
 * name instead; see config.c. */
static const char *const builtin_hosts[] = {
    /* music */
    "music.youtube.com",
    "open.spotify.com",
    "spotify.com",
    "www.spotify.com",
    "soundcloud.com",
    "www.soundcloud.com",
    "m.soundcloud.com",
    "music.apple.com",
    "deezer.com",
    "www.deezer.com",
    "tidal.com",
    "listen.tidal.com",
    "music.amazon.com",
    "pandora.com",
    "www.pandora.com",
    "bandcamp.com",
    "jiosaavn.com",
    "www.jiosaavn.com",
    "gaana.com",
    "www.gaana.com",
    "audiomack.com",
    "www.audiomack.com",
    /* Discord */
    "discord.com",
    "www.discord.com",
    "ptb.discord.com",
    "canary.discord.com",
    "discordapp.com",
    "www.discordapp.com",
    "discord.gg",
    "gateway.discord.gg",
    "discordapp.net",
    "cdn.discordapp.com",
    "media.discordapp.net",
    /* AI chat */
    "chatgpt.com",
    "www.chatgpt.com",
    "chat.openai.com",
    "openai.com",
    "www.openai.com",
    "claude.ai",
    "www.claude.ai",
    "gemini.google.com",
    "aistudio.google.com",
    "copilot.microsoft.com",
    "perplexity.ai",
    "www.perplexity.ai",
    "chat.deepseek.com",
    "deepseek.com",
    "www.deepseek.com",
    "grok.com",
    "www.grok.com",
    "x.ai",
    "chat.mistral.ai",
    "poe.com",
    "www.poe.com",
    "openrouter.ai",
    "www.openrouter.ai",
    "meta.ai",
    "www.meta.ai",
    "you.com",
    "www.you.com",
    "phind.com",
    "www.phind.com",
    "duck.ai",
    /* AI APIs, so editor and terminal assistants stop too */
    "api.openai.com",
    "api.anthropic.com",
    "generativelanguage.googleapis.com",
    "api.deepseek.com",
    "api.x.ai",
    "api.mistral.ai",
    "api.perplexity.ai",
    "api.groq.com",
    "api.together.xyz",
    "api.cohere.com",
    "api.githubcopilot.com",
    "copilot-proxy.githubusercontent.com",
    "api2.cursor.sh",
    "server.codeium.com",
};

bool host_valid(const char *host) {
    size_t len = strlen(host);
    bool label_start = true;
    int dots = 0;
    for (const char *c = host; *c != '\0'; c++) {
        bool edge = label_start || c[1] == '.' || c[1] == '\0';
        if (*c == '.') {
            dots++;
        }
        if (!isalnum((unsigned char)*c) && (edge || strchr(".-", *c) == NULL)) {
            return false;
        }
        label_start = *c == '.';
    }
    return dots >= 1 && len < CONFIG_HOST_MAX;
}

static bool allowed(const struct config *cfg, const char *host) {
    for (int i = 0; i < cfg->allow_count; i++) {
        if (text_same_nocase(cfg->allow[i], host)) {
            return true;
        }
    }
    return false;
}

static int put_host(const struct config *cfg, const char *host, struct sbuf *out) {
    if (allowed(cfg, host)) {
        return 0;
    }
    sbuf_putf(out, "0.0.0.0 %s\n:: %s\n", host, host);
    return 1;
}

static bool blocked(const struct config *cfg, const char *host) {
    if (allowed(cfg, host)) {
        return false;
    }
    for (size_t i = 0; i < sizeof builtin_hosts / sizeof builtin_hosts[0]; i++) {
        if (text_same_nocase(builtin_hosts[i], host)) {
            return true;
        }
    }
    for (int i = 0; i < cfg->block_count; i++) {
        if (text_same_nocase(cfg->block[i], host)) {
            return true;
        }
    }
    return false;
}

/* True when a hosts line gives an address to a blocked host. The resolver
 * returns every address listed for a name, so such a line would hand the
 * browser a working address right after the null route. */
static bool line_names_blocked(const struct config *cfg, const char *line, size_t length) {
    char copy[512], *save = NULL;
    if (length >= sizeof copy) {
        return false;
    }
    memcpy(copy, line, length);
    copy[length] = '\0';
    copy[strcspn(copy, "#")] = '\0';
    for (const char *word = strtok_r(copy, " \t\r", &save); word != NULL;
         word = strtok_r(NULL, " \t\r", &save)) {
        if (blocked(cfg, word)) {
            return true;
        }
    }
    return false;
}

/* Copies `original`, leaving out any block an earlier call put in it and any
 * line that names a blocked host. */
static void put_original(const struct config *cfg, const char *original, struct sbuf *out) {
    const char *start = strstr(original, blocklist_marker);
    const char *end = start != NULL ? strstr(start, blocklist_end) : NULL;
    const char *skip_to = end != NULL ? end + strlen(blocklist_end) : NULL;
    for (const char *line = original; *line != '\0';) {
        size_t length = strcspn(line, "\n");
        size_t whole = length + (line[length] == '\n');
        if (end != NULL && line == start) {
            line = skip_to;
            continue;
        }
        if (!line_names_blocked(cfg, line, length)) {
            sbuf_putn(out, line, whole);
        }
        line += whole;
    }
}

int blocklist_render(const struct config *cfg, const char *original, struct sbuf *out) {
    int count = 0;
    sbuf_put(out, blocklist_marker);
    for (size_t i = 0; i < sizeof builtin_hosts / sizeof builtin_hosts[0]; i++) {
        count += put_host(cfg, builtin_hosts[i], out);
    }
    for (int i = 0; i < cfg->block_count; i++) {
        count += put_host(cfg, cfg->block[i], out);
    }
    sbuf_put(out, blocklist_end);
    put_original(cfg, original, out);
    return count;
}
