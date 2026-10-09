#include "core/blocklist.h"
#include "core/config.h"
#include "suites.h"
#include "test.h"

#include <string.h>

static bool parse(struct config *c, const char *source, char *problems, size_t cap) {
    char text[8192];
    struct sbuf sb = sbuf_on(problems, cap);
    config_defaults(c);
    text_copy(text, sizeof text, source);
    return config_parse(c, text, &sb);
}

static void defaults_match_the_research_protocol(void) {
    struct config c;
    config_defaults(&c);
    CHECK_INT(c.slot_am, 11 * 60);
    CHECK_INT(c.slot_pm, 23 * 60);
    CHECK_INT(c.hard_stop, 2 * 60);
    CHECK_INT(c.commit_minutes, 10);
    CHECK_INT(c.plan_count, 3);
    CHECK_INT(c.kill_count, 6);
    CHECK_STR(c.kill[0], "Discord");
}

static void the_first_run_template_is_itself_valid(void) {
    struct config c;
    char problems[512];
    CHECK(parse(&c, config_template, problems, sizeof problems));
    CHECK_STR(problems, "");
    CHECK_INT(c.plan_count, 3);
}

static void reads_every_setting(void) {
    struct config c;
    char problems[512];
    CHECK(parse(&c,
                "slot_am = 10:30\nslot_pm = 22:15\nhard_stop = 01:00\ncommit_minutes = 5\n"
                "scale = 2\nproject = /home/z/kilo\nplan = If A, then B.\nplan = If C, then D.\n"
                "block = example.com\nallow = soundcloud.com\nkill = telegram-desktop\n",
                problems, sizeof problems));
    CHECK_INT(c.slot_am, 10 * 60 + 30);
    CHECK_INT(c.slot_pm, 22 * 60 + 15);
    CHECK_INT(c.hard_stop, 60);
    CHECK_INT(c.commit_minutes, 5);
    CHECK_INT(c.scale, 2);
    CHECK_STR(c.project, "/home/z/kilo");
    CHECK_INT(c.plan_count, 2);
    CHECK_STR(c.plans[1], "If C, then D.");
    CHECK_STR(c.block[0], "example.com");
    CHECK_STR(c.allow[0], "soundcloud.com");
    CHECK_INT(c.kill_count, 7);
    CHECK_STR(c.kill[6], "telegram-deskto");
}

static void lists_every_problem_with_its_line_number(void) {
    struct config c;
    char problems[2048];
    CHECK(!parse(&c,
                 "slot_am = 25:00\ncommit_minutes = 0\nscale = 9\nproject = relative/path\n"
                 "colour = blue\nblock = not a host\nkill = bad name!\nplan =\nallow = x\n"
                 "commit_minutes = ten\nhard_stop = 2\nslot_pm = noon\n",
                 problems, sizeof problems));
    CHECK_HAS(problems, "line 1: slot_am = \"25:00\" is not valid. Expected a time like 11:00.");
    CHECK_HAS(problems, "line 2: commit_minutes");
    CHECK_HAS(problems, "line 3: scale");
    CHECK_HAS(problems, "line 4: project");
    CHECK_HAS(problems, "line 5: unknown setting \"colour\".");
    CHECK_HAS(problems, "line 6: block");
    CHECK_HAS(problems, "line 7: kill");
    CHECK_HAS(problems, "line 8: plan");
    CHECK_HAS(problems, "line 9: allow");
    CHECK_HAS(problems, "line 10: commit_minutes");
    CHECK_HAS(problems, "line 11: hard_stop");
    CHECK_HAS(problems, "line 12: slot_pm");
    CHECK(!parse(&c, "commit_minutes = 121\ncommit_minutes =\nkill =\nscale = -1\n", problems,
                 sizeof problems));
    CHECK_HAS(problems, "line 1:");
    CHECK_HAS(problems, "line 2:");
    CHECK_HAS(problems, "line 3:");
    CHECK_HAS(problems, "line 4:");
}

static void refuses_values_that_are_too_long(void) {
    struct config c;
    char problems[4096], text[4096];
    struct sbuf sb = sbuf_on(text, sizeof text);
    sbuf_put(&sb, "project = /");
    for (int i = 0; i < 30; i++) {
        sbuf_put(&sb, "0123456789");
    }
    sbuf_put(&sb, "\nplan = ");
    for (int i = 0; i < 25; i++) {
        sbuf_put(&sb, "0123456789");
    }
    sbuf_put(&sb, "\nkill = ");
    for (int i = 0; i < 7; i++) {
        sbuf_put(&sb, "0123456789");
    }
    sbuf_put(&sb, "\n");
    for (int i = 0; i < 65; i++) {
        sbuf_putf(&sb, "allow = host%d.example.com\n", i);
    }
    CHECK(!parse(&c, text, problems, sizeof problems));
    CHECK_HAS(problems, "line 1: project");
    CHECK_HAS(problems, "line 2: plan");
    CHECK_HAS(problems, "line 3: kill");
    CHECK_HAS(problems, "line 68: allow");
}

static void refuses_equal_slots_and_overfull_lists(void) {
    struct config c;
    char problems[8192], text[8192];
    struct sbuf many = sbuf_on(text, sizeof text);
    CHECK(!parse(&c, "slot_am = 09:00\nslot_pm = 09:00\n", problems, sizeof problems));
    CHECK_HAS(problems, "same time");
    for (int i = 0; i < 65; i++) {
        sbuf_putf(&many, "block = host%d.example.com\n", i);
    }
    for (int i = 0; i < 11; i++) {
        sbuf_putf(&many, "kill = name%d\n", i);
    }
    sbuf_put(&many, "plan = one\nplan = two\nplan = three\nplan = four\n");
    CHECK(!parse(&c, text, problems, sizeof problems));
    CHECK_HAS(problems, "line 65: block");
    CHECK_HAS(problems, "line 76: kill");
    CHECK_HAS(problems, "line 80: plan");
    CHECK_INT(c.block_count, 64);
    CHECK_INT(c.kill_count, 16);
}

static void a_host_must_be_a_plain_dns_name(void) {
    static const char *const good[] = {"example.com", "a.b.c.example.co.uk", "x-y.example.com",
                                       "api2.cursor.sh"};
    static const char *const bad[] = {
        "",
        "localhost",
        "exa mple.com",
        ".example.com",
        "example.com.",
        "a..b",
        "-a.com",
        "a-.com",
        "a.com\n0.0.0.0 evil.com",
        "a.com #x",
        "http://a.com",
        "a.com/path",
        "*.example.com",
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.com",
    };
    for (size_t i = 0; i < sizeof good / sizeof good[0]; i++) {
        CHECK(host_valid(good[i]));
    }
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        CHECK(!host_valid(bad[i]));
    }
}

static void block_list_keeps_the_original_and_adds_both_address_families(void) {
    struct config c;
    char out[64 * 1024];
    struct sbuf sb = sbuf_on(out, sizeof out);
    config_defaults(&c);
    int count = blocklist_render(&c, "127.0.0.1 localhost", &sb);
    CHECK(strncmp(out, blocklist_marker, strlen(blocklist_marker)) == 0);
    CHECK_HAS(out, "# lockin: end of block\n127.0.0.1 localhost");
    CHECK_HAS(out, "\n0.0.0.0 music.youtube.com\n:: music.youtube.com\n");
    CHECK_HAS(out, "0.0.0.0 discord.com\n");
    CHECK_HAS(out, "0.0.0.0 claude.ai\n");
    CHECK_HAS(out, "0.0.0.0 api.openai.com\n");
    CHECK_HAS(out, "0.0.0.0 openrouter.ai\n");
    CHECK(count > 70);
    CHECK(!sb.truncated);
}

static void block_list_honours_block_and_allow_settings(void) {
    struct config c;
    char problems[256], out[64 * 1024];
    struct sbuf sb = sbuf_on(out, sizeof out);
    CHECK(parse(&c, "block = example.com\nallow = SoundCloud.com\n", problems, sizeof problems));
    blocklist_render(&c, "", &sb);
    CHECK_HAS(out, "0.0.0.0 example.com\n");
    CHECK(strstr(out, "0.0.0.0 soundcloud.com\n") == NULL);
    CHECK_HAS(out, "0.0.0.0 www.soundcloud.com\n");
    CHECK(strncmp(out, blocklist_marker, strlen(blocklist_marker)) == 0);
}

static void rendering_twice_does_not_stack_the_block(void) {
    struct config c;
    char first[64 * 1024], second[64 * 1024];
    struct sbuf a = sbuf_on(first, sizeof first);
    struct sbuf b = sbuf_on(second, sizeof second);
    config_defaults(&c);
    blocklist_render(&c, "127.0.0.1 localhost\n", &a);
    blocklist_render(&c, first, &b);
    CHECK_STR(second, first);
    /* A marker with no end line is someone else's text: keep it. */
    b = sbuf_on(second, sizeof second);
    blocklist_render(&c, blocklist_marker, &b);
    CHECK(strstr(strstr(second, blocklist_end), blocklist_marker) != NULL);
}

static void an_existing_address_for_a_blocked_host_is_dropped(void) {
    struct config c;
    char out[64 * 1024], wide[700];
    struct sbuf sb = sbuf_on(out, sizeof out);
    config_defaults(&c);
    blocklist_render(&c,
                     "127.0.0.1 localhost\n160.79.104.10\tAPI.Anthropic.com alias\n"
                     "10.0.0.5 intranet # discord.com is fine in a comment\n10.0.0.6 nas",
                     &sb);
    CHECK(strstr(out, "160.79.104.10") == NULL);
    CHECK_HAS(out, "127.0.0.1 localhost\n10.0.0.5 intranet # discord.com");
    CHECK_HAS(out, "10.0.0.6 nas");
    text_copy(c.block[0], CONFIG_HOST_MAX, "nas");
    text_copy(c.block[1], CONFIG_HOST_MAX, "intranet");
    text_copy(c.allow[0], CONFIG_HOST_MAX, "intranet");
    c.block_count = 2;
    c.allow_count = 1;
    sb = sbuf_on(out, sizeof out);
    blocklist_render(&c, "10.0.0.5 intranet\n10.0.0.6 nas\n", &sb);
    CHECK_HAS(out, "10.0.0.5 intranet\n");
    CHECK(strstr(out, "10.0.0.6 nas") == NULL);
    c.block_count = 0;
    c.allow_count = 0;
    memset(wide, 'a', sizeof wide - 1);
    wide[sizeof wide - 1] = '\0';
    sb = sbuf_on(out, sizeof out);
    blocklist_render(&c, wide, &sb);
    CHECK_HAS(out, wide);
}

void suite_config(void) {
    RUN(refuses_values_that_are_too_long);
    RUN(an_existing_address_for_a_blocked_host_is_dropped);
    RUN(defaults_match_the_research_protocol);
    RUN(the_first_run_template_is_itself_valid);
    RUN(reads_every_setting);
    RUN(lists_every_problem_with_its_line_number);
    RUN(refuses_equal_slots_and_overfull_lists);
    RUN(a_host_must_be_a_plain_dns_name);
    RUN(block_list_keeps_the_original_and_adds_both_address_families);
    RUN(block_list_honours_block_and_allow_settings);
    RUN(rendering_twice_does_not_stack_the_block);
}
