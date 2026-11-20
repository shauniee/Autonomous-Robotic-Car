# Project guidance

## Contents

- [Guardrails](#guardrails)
- [Where to look](#where-to-look)

Guardrails below always apply. Everything else lives in the guide listed under [Where to look](#where-to-look); read only the guide the task needs.

## Guardrails

- **Facts:** each fact has one owning guide; other files link to it. Keep requirements, proposed design, implemented code and observed results distinct. Never invent course rules, calibrated limits, measurements or module ratings; unknown values go to the [pending decisions](docs/project-specs.md#pending-decisions).
- **Code:** follow the [coding rules](docs/team.md#5-coding-rules). Only Motion writes the motors; networking never starts, stops or drives the robot ([authority](docs/architecture.md#authority)).
- **Git:** one branch (`<type>/<slug>`: `feat`, `fix`, `chore`, `docs`, `test`) = one rebase-merged PR into `main`, each commit authored by the owner of the subsystem it touches. PR titles, commits and issue titles are Conventional Commits with a scope (`fix(mission): …`; types `feat`, `fix`, `chore`, `docs`, `test`, `refactor`), checked by CI. PR bodies follow `.github/pull_request_template.md` and say `Closes #N`; new issues use the forms in `.github/ISSUE_TEMPLATE/`. Never commit to `main`; never force-push `main` or delete someone's branch. Push, open a PR or merge only when asked.
- **No co-authors:** never add `Co-Authored-By` or any co-author attribution to a commit message, PR or merge message.
- **Never commit** `config/wifi_secrets.h`, credentials, `*.elf`, `*.uf2`, `*.map` or video files.
- **Testing:** follow [testing](testing/README.md): up to five unit tests per subsystem (`S1-U01` to `S5-U05`) and one system test, [SYS-U01 Full course run](testing/system/README.md#sys-u01-full-course-run). Each test is shown on [video](video/README.md); a test goes in the folder of the subsystem it exercises, `system/` for anything that needs two or more. Append one result row per run with its log in that folder's `logs/`, and record the exact commit. Never reuse a test ID. A build or host test is not a firmware or physical pass; do not save build or host output.
- **Markdown:** `docs/` holds only `project-specs`, `team`, `architecture` and `tribunal1`; subsystem detail goes in each `app_program/<name>/README.md`. Every Markdown file except the `.github/` templates starts with a contents list of its sections.

## Where to look

| Need | Guide |
| --- | --- |
| All open work and planned changes | [GitHub issues](https://github.com/shauniee/Autonomous-Robotic-Car/issues) |
| Build, flash, calibrate, MQTT laptop tool, bring-up order | [README](README.md) |
| Requirements, minimum-bit change requirement, pending decisions | [project-specs](docs/project-specs.md) |
| Bits used by each field, and the evidence for them | [team](docs/team.md#6-data-representation) |
| Tasks, authority, shared data, fault policy, MQTT frame layouts | [architecture](docs/architecture.md) |
| Members, pins, wiring, task budget, interfaces, coding rules | [team](docs/team.md) |
| One subsystem: comms, motion, line_barcode, terrain, obstacle | `app_program/<name>/README.md` (`comms`, `motion`, `line_barcode`, `terrain`, `obstacle`) |
| Test IDs, how to record results, host tests | [testing](testing/README.md) |
| Is the robot ready for a full run? | [mission-readiness checklist](testing/system/mission-readiness.md) |
| Line-tracking state, open issues and how to run a trial | [line_barcode README](app_program/line_barcode/README.md) |
| Video recording | [video](video/README.md) |
