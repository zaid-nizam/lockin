# Use cases

One actor: the user at their own machine. "Unit" is a test in `tests/test_*.c`
(fake system); "system" is `tests/linux_tests.c`; "e2e" is a scenario in
`tests/e2e.sh` (real X server, real mount).

| # | Action and result | Unhappy paths | Unit | System / e2e |
|---|---|---|---|---|
| 1 | Start with `sudo lockin`: window opens on the nearest slot | no display; 8-bit display; folder cannot be created; sudo for an unknown user; bad `SUDO_UID` | `opens_on_the_start_screen_for_the_nearest_slot` | e2e "no display", "8-bit display", "a folder that cannot be created"; system `no-user`, `bad-uid`, `bad-dir` |
| 2 | Start without root: look around, Lock in disabled | clicking Lock in anyway | `without_root_the_lock_button_is_dead_and_says_why` | e2e "ordinary user"; system `unprivileged` |
| 3 | Edit settings, `lockin --check` | every kind of bad value; unknown key; unreadable file | `lists_every_problem_with_its_line_number`, `refuses_values_that_are_too_long`, `unreadable_settings_stop_the_start` | e2e "command line" |
| 4 | **Lock in: sites blocked until reboot, programs closed, session starts.** The one that must not fail | block fails; hosts unreadable; hosts over 64 KB; state cannot be saved; Enter in a text box; right click; pressed twice; a host already has an address in hosts | `locking_blocks_sites_and_starts_the_session`, `a_failed_block_stays_put_and_explains`, `an_oversized_hosts_file_is_refused_not_truncated`, `enter_in_a_text_box_never_locks`, `an_existing_address_for_a_blocked_host_is_dropped` | system `block`, `sudo`, `block-fails`; e2e "full session" |
| 5 | Try to leave while locked: window stays, signals ignored | close button; Ctrl-C; SIGTERM; SIGHUP; program killed outright (block stays) | `the_window_closes_only_before_locking` | system `refuse`; e2e "full session", "restart while blocked" |
| 6 | Press Urge +1 when the pull comes | | `urges_are_counted` | e2e "full session" |
| 7 | Watch the 10-minute commitment and the hard stop | clock past hard stop | `the_commitment_bar_fills_over_ten_minutes`, `the_session_header_warns_after_the_hard_stop` | |
| 8 | End the session: `NEXT.md` written, one row logged | empty form; each bad field; NEXT.md write fails; log write fails; state write fails; Back; Escape; Save twice | `saving_an_empty_form_lists_every_problem`, `a_valid_form_writes_next_md_and_one_log_row`, `a_failed_save_keeps_the_form_open`, `back_and_escape_return_to_the_session` | e2e "full session" |
| 9 | Log a missed slot | log write fails; memory full of rows | `a_missed_slot_is_logged_and_the_window_can_close`, `a_full_log_in_memory_drops_its_oldest_row` | e2e "missed slot" |
| 10 | Second session in one boot | block already in place | `a_second_session_reuses_the_block_without_stacking_it` | system `block` (second call) |
| 11 | Read Stats: medians, verdicts, month targets | no data; too little data; damaged rows in the file | `stats_*`, `no_verdict_before_enough_sessions`, `rows_load_skips_the_header_and_junk` | e2e "full session" opens it |
| 12 | Restart later: log, week, coin and NEXT.md are picked up | unreadable files | `a_restart_picks_up_the_log_the_week_and_the_next_step`, `an_unreadable_file_is_logged_and_treated_as_empty` | e2e "restart while blocked" |
| 13 | It crashes: a stack trace is in `lockin.log` | | | system `crash`; e2e "crash" |
| 14 | HiDPI screen: the window scales | | | e2e "scale = 2", "tall screens" |

The "someone else's resource" cases of a multi-user service do not apply:
there is one user. The closest equivalent, root acting on another user's
processes or files, is in `THREATS.md` and tested by system `sudo`.
