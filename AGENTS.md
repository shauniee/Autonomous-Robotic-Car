# Project guidance

## Contents

- [Guardrails](#guardrails)
- [Where to look](#where-to-look)

Guardrails below always apply. Everything else lives in the guide listed under [Where to look](#where-to-look); read only the guide the task needs.

## Guardrails

- **Facts:** each fact has one owning guide; other files link to it. Keep requirements, proposed design, implemented code and observed results distinct. Never invent course rules, calibrated limits, measurements or module ratings; unknown values go to the [pending decisions](docs/project-specs.md#pending-decisions).
- **Code:** follow the [coding rules](docs/team.md#5-coding-rules). Only Motion writes the motors; networking never starts, stops or drives the robot ([authority](docs/architecture.md#authority)).
- **Git:** one branch (`<type>/<slug>`: `feat`, `fix`, `chore`, `docs`, `test`) = one squash-merged PR into `main`. Never commit to `main`, `uat` or `legacy-main`; never force-push `main` or delete someone's branch. Push, open a PR or merge only when asked.
- **Never commit** `config/wifi_secrets.h`, credentials, `*.elf`, `*.uf2`, `*.map` or video files.
- **Testing:** keep fixtures and evidence in `testing/`. Append results; never overwrite evidence or reuse a test ID. A build or host test is not a firmware or physical pass; record the exact commit with each result ([testing](testing/README.md#run-and-record)).
- **Markdown:** lowercase file stems under `docs/`. Every Markdown file starts with a contents list of its sections.

## Where to look

| Need | Guide |
| --- | --- |
| Full feature implementation sequence and source baseline | [plan](plan.md) |
| Build, flash, calibrate, MQTT laptop tool, bring-up order | [README](README.md) |
| Requirements, minimum-bit change requirement, data table, pending decisions | [project-specs](docs/project-specs.md) |
| Tasks, authority, shared data, fault policy, MQTT frame layouts | [architecture](docs/architecture.md) |
| Members, pins, wiring, task budget, interfaces, coding rules | [team](docs/team.md) |
| One subsystem in depth: S1 comms, S2 motion, S3 line/barcode/Mission, S4 terrain, S5 obstacle | `docs/architecture/subsystem1.md` .. `subsystem5.md`; code in `app_program/subsystemN/` |
| Test IDs, how to record results, host tests | [testing](testing/README.md) |
| Is the robot ready for a full run? | [mission-readiness checklist](testing/mission-readiness.md) |
| Deferred Week 13 work and video recording | [week13-plan](docs/week13-plan.md) |
| Week 8 Tribunal decisions | [tribunal1](docs/additional-docs/tribunal1.md) |
