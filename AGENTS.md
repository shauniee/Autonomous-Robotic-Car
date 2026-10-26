# Project guidance

Follow [coding rules](docs/team.md#7-coding-rules). Record course/calibration decisions once in [project-specs](docs/project-specs.md#pending-decisions), then update the owning guide.

## Owning guides

Each fact has one owner; other guides link to it.

| Topic | Owner |
| --- | --- |
| Navigation, attribution | [README](README.md) |
| Members, pins, peripherals, hardware limits, workflow, coding rules | [team](docs/team.md) |
| Requirements, deliverables, pending decisions (single register) | [project-specs](docs/project-specs.md) |
| System coordination, shared-data rules | [architecture](docs/architecture.md) |
| Data widths and scales (data table) | [project-specs](docs/project-specs.md) |
| Test IDs, results, evidence | [testing](testing/README.md) |

Subsystem guides are `docs/architecture/subsystem1.md`..`subsystem5.md` (S1 comms, S2 motion, S3 line/barcode/mission, S4 terrain, S5 obstacle); tribunal summary in `docs/additional-docs/tribunal1.md`. Code is in `app_program/subsystemN/`.

## Scope and ownership

Nicholas owns S3 line/barcode and Mission/Safety. Only a subsystem's owner edits its folders (see [team](docs/team.md)); cross-subsystem implementation needs explicit user authorization for that task. Do not extend one task's authorization to the next.

## Content rules

- Keep requirements, proposed behavior, implemented software and observations distinct. Do not invent course rules, calibrated limits or module ratings.
- Active guides contain no PDF filenames, transcript filenames or page references.
- Leave `videos/README.md` byte-for-byte unchanged (its `docs/GETTING_STARTED.md` link is known-dangling after the docs housekeeping).
- Lowercase Markdown filename stems under `docs/`.
- Never commit `config/wifi_secrets.h`, credentials, `*.elf`/`*.uf2`/`*.map`, or videos (all git-ignored).

## Testing

- Use `testing/` exclusively for active fixtures, definitions and evidence; no host fake-kernel runners.
- Append real results and failures; never overwrite evidence. Preserve stable IDs and meanings; reserve retired IDs.
- A build is not an executed firmware or physical pass. Record the exact committed revision with each result, as [testing](testing/README.md) specifies.

## Issue branch workflow

One GitHub issue = one branch = one pull request = one squash merge into `main`.

1. **Issue first.** Work starts from an open issue. If none exists, propose a title, labels and scope to the user; do not create issues unasked.
2. **Branch.** Create it from current `main` and name it exactly as the issue title, `<type>/<slug>` (e.g. `docs/docs-cleanup`, `feat/min-bit-s4`, `test/s3-physical-ir-barcode`). Type is one of `feat`, `fix`, `chore`, `docs`, `test`. Never commit to `main`, `uat` or `legacy-main`.
3. **Stay in scope.** Touch only the issue's Scope; respect its Out of scope and Depends on. Do not mix issues on one branch. If the work needs an unmerged issue, stop and report instead of stacking branches.
4. **Commit freely, in small steps.** Each commit has one logical change and a prefix: `feat:`, `fix:`, `chore:`, `docs:` or `test:` (optional scope, e.g. `test(s3):`). History is squashed, so intermediate commits may be granular but must be accurate.
5. **Verify before handing off.** Run the smallest proof the change allows (build profile, link check, test record) and state what was and was not executed.
6. **Hand off, do not merge.** Push the branch and open or update the PR only when the user asks. The user performs the **squash merge**; never merge, rebase onto, or force-push `main`, and never delete the user's branches.
7. **Squash message.** Propose it as `<prefix>: <summary> (#<PR>)`, with a body summarizing the net change and `Closes #<issue>`. Merge commits are not used.
8. **After merge.** The next issue branches from the updated `main`. Do not reuse a merged branch; squash rewrites its commits, so stale branches conflict.

## Housekeeping

- `docs/team.md` is current.
- Tracking issue for repository realignment: #62.
