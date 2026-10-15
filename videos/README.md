# Videos

Short videos of tests and demos running on the real hardware. A video shows
what a log cannot: the LED blinking, the motor turning, the robot reaching
the line. List **every** video in the table at the bottom of this page.

## What to record

- **Short:** 15–60 seconds. One test or one feature per video.
- **Say what it is.** Show or say the test ID (e.g. `S2-U03`, `INT-01`) or
  the demo name at the start.
- **Show the commit.** Before recording, run `git rev-parse --short HEAD` and
  put it in the table below and record the flashed-image checksum in the linked test result.
- **Show the result.** Keep both the hardware and, if it matters, the
  USB-only TEST console in view. Use camera/scope or wireless evidence during battery-only trials under the [power policy](../docs/GETTING_STARTED.md#power-and-flashing).
- **Landscape**, steady, good light. No music needed.

## How to share it: unlisted YouTube only

1. Upload the video to YouTube with visibility set to **Unlisted**: anyone
   with the link can watch it, but it does not show up in search. Not
   *Private* (only invited accounts can watch, so the link won't work for
   your team or instructor), and not *Public*.
2. Copy the video's link (e.g. `https://youtu.be/...`).

**Do not commit video files to this repo or attach them on GitHub.** A video
in the repo stays in the git history for good, even if deleted later, and
slows every clone for the whole team. Video files are ignored by
`.gitignore` for this reason.

Then add a row to the table below. If the video is evidence for a test, also
put its link in the *Log / evidence* column of that test's row in
[test records](../testing/README.md).

## Video index

Newest at the bottom. Never delete a recorded row. The example row below is a template placeholder, not recorded evidence.

| Date | Subsystem | Test ID / demo | Commit | Student ID | Link | Notes |
|---|---|---|---|---|---|---|
| YYYY-MM-DD | S1 | S1-U03 | `abc1234` | | <https://youtu.be/...> | LED blinks at 2 Hz |

## UAT evidence

No hardware video was recorded during the current software completion. Add real UAT feature/rehearsal links with test IDs, exact revision and image checksum after the physical passes. Preserve existing rows and never promote the placeholder row into a recorded result.
