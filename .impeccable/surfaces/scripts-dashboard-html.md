---
version: 1
slug: "scripts-dashboard-html"
primary_target: "scripts/dashboard.html"
related_targets: ["scripts/mqtt_client.py"]
---

# Surface: Robot Telemetry dashboard

## Contents

- [Scope](#scope)
- [Direction contract](#direction-contract)
- [Acceptance](#acceptance)

## Scope

Target: `scripts/dashboard.html`, served by `scripts/mqtt_client.py serve`.
Mode: Operate. Audience: CSC01 team beside the robot at a lab bench.
Product constraints live in [PRODUCT.md](../../PRODUCT.md); visual tokens and
interaction rules in [DESIGN.md](../../DESIGN.md).

## Direction contract

One-screen bench ledger. Retain the dark board, condensed identity details
and meaningful lamps; use readable system-sans data and atomic updates.
First viewport: all seven Mission states with only the reported state marked;
Mission faults in the top strip; five subsystem health/fault rows in S1–S5 order; every existing decoded
sensor reading; four custom barcode letters and ACK; junction tests; speed
chart and bounded event log. Configuration occupies a fixed right column.

No sequential Mission progress, invented telemetry or remote start/stop.
Mapping drafts and installed status are distinct. Stale data stays visible.

## Acceptance

The user selected 1440 × 900 on 2026-10-31. The whole normal dashboard must fit
without document scrolling at that viewport. Fault text may grow naturally;
never clip fault information to preserve a height target. Smaller windows
reflow and scroll. No dependencies, build step, external fonts or images.
Verify rendered live/empty/stale/fault fixtures and control gating; preserve
evidence under the project's testing policy.
