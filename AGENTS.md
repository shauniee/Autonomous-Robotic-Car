# Project guidance

Follow [coding rules](docs/team.md#5-coding-rules). Record course/calibration decisions once in [project-specs](docs/project-specs.md#pending-decisions), then update the owning guide.

## Owning guides

Each fact has one owner; other guides link to it.

| Topic | Owner |
| --- | --- |
| Navigation, getting started (build, flash, bring-up), attribution | [README](README.md) |
| Members, pins and wiring, task budget, interfaces, coding rules | [team](docs/team.md) |
| Requirements, deliverables, pending decisions (single register) | [project-specs](docs/project-specs.md) |
| System coordination, shared-data rules | [architecture](docs/architecture.md) |
| Data widths and scales (data table) | [project-specs](docs/project-specs.md) |
| Test IDs, results, evidence | [testing](testing/README.md) |

Subsystem guides are `docs/architecture/subsystem1.md`..`subsystem5.md` (S1 comms, S2 motion, S3 line/barcode/mission, S4 terrain, S5 obstacle); tribunal summary in `docs/additional-docs/tribunal1.md`. Code is in `app_program/subsystemN/`.

## Content rules

- Keep requirements, proposed behavior, implemented software and observations distinct. Do not invent course rules, calibrated limits or module ratings.
- Leave `videos/README.md` byte-for-byte unchanged (its `docs/GETTING_STARTED.md` link is known-dangling after the docs housekeeping).
- Lowercase Markdown filename stems under `docs/`.

## Workflow

One GitHub issue = one branch = one PR = one squash merge into `main`. Branch from current `main` as `<type>/<slug>` (`feat`, `fix`, `chore`, `docs`, `test`); commit with the same prefixes; stay in the issue's scope. Never commit to `main`, `uat` or `legacy-main`. Push, open a PR and merge only when the user asks; never force-push `main` or delete the user's branches. Never commit `config/wifi_secrets.h`, credentials, `*.elf`/`*.uf2`/`*.map` or videos.

## Testing

- Use `testing/` exclusively for active fixtures, definitions and evidence; no host fake-kernel runners.
- Append real results and failures; never overwrite evidence. Preserve stable IDs and meanings; reserve retired IDs.
- A build is not an executed firmware or physical pass. Record the exact committed revision with each result, as [testing](testing/README.md) specifies.