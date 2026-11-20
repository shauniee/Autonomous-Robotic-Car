<!--
Title: a Conventional Commit with a scope, e.g. fix(mission): commit a queued turn on two both-black samples
Types: feat, fix, chore, docs, test, refactor. PRs are rebase-merged, so every commit subject must follow the same format.
No co-author or "Generated with" lines anywhere.
-->

## What

## Why

Closes #

## How it was tested

<!-- On the car: test ID, commit and log path. Host tests and builds are not a car pass; say what was not run. -->

## Checklist

- [ ] No `config/wifi_secrets.h`, credentials, `*.elf`, `*.uf2`, `*.map` or video files in the diff
- [ ] `python3 scripts/check_barr.py --all` passes
- [ ] Host tests added or updated (`bash testing/tools/run.sh`)
- [ ] Car runs recorded in the test log, with the exact commit
- [ ] Docs, subsystem README or AGENTS.md updated if behaviour or commands changed
