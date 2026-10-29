---
version: 1
slug: "scripts-dashboard-html"
primary_target: "scripts/dashboard.html"
related_targets: ["scripts/mqtt_client.py"]
---

# Surface: Robot Telemetry dashboard

Scope: `scripts/dashboard.html` served by `scripts/mqtt_client.py serve`. Mode: Operate.

Audience and job: CSC01 team at the lab bench, standing beside the robot, half-watching it. Read health in one glance; enter the four mission-day barcode letters and send them while the robot is stopped.

Constraints: one static file, offline (no CDN), loopback server, networking never starts/stops/drives the robot; mapping accepted only while READY/STOPPED.

Unresolved: instructor broker accepting the mapping on demo day (D09); tuning build (separate, not here).

## Direction contract

THESIS: The robot's state is a departure board: fixed rows per subsystem, fixed columns, and only the cells change. It refuses the card-grid telemetry dashboard of floating tiles.

OWN-WORLD: Near-black board (#121212) with matte cell faces (#1d1d1d) and 2px gutters; white condensed capitals in fixed cells; amber (#ffb000) row lamp for warning, stale and pending; red (#c8352b) for stop and fault; cleared states dim, never vanish. Standard buttons and inputs styled as cells.

STORY: The team sees at once whether the robot is healthy and which subsystem is not, enters four letters, sends them, and reads the robot's acknowledgement on the same strip.

FIRST VIEWPORT: Top bar: title left, link/robot/frame-age lamps right. Mapping strip full width beneath: four large letter cells (Left, Right, Straight, U-turn), Send mapping button, ACK readout and installed state. Then the subsystem board table (Mission, Line IR, Motion, Obstacle, Terrain, Network) with State / Reading / Detail / Fault columns, Mission first. Below: wheel-speed chart and the event and acknowledgement log as a departures list.

FORM: Split-flap concourse board, grounded list position: challenger (dealt), chosen over assigned. seed:00876d814f842eae. Signature move: a changed cell flips character by character once (reduced motion: plain update).

FINISH: unreviewed and undocumented is unfinished; this build ends with the finish review, the verdict, DESIGN.md, and every shipping raster carrying its provenance

## Cited adaptations

- Red: #c8352b is the lamp dot and the large bold State text (about 3.1:1 on the #3a1512 wash, passing 3:1 for large text). Smaller red text (fault cells, and State on phones at 16px) uses #ff5a4a for 4.5:1. The row wash is #3a1512.
- Fault cells hold one line: active faults in red, otherwise faults cleared this run dimmed with "cleared"; the full list is in the cell's tooltip.
- Green lamp and OK state are kept for healthy rows and an installed mapping; amber stays reserved for warning, stale and pending.
- Desktop only (confirmed 2026-10-29): phones and tablets are not targets. The 1100px and 720px rules exist only for narrow laptop windows; at 720px the Detail column drops as the least urgent column.
- Face: a condensed system stack (DIN Condensed, Bahnschrift, Arial Narrow, Roboto Condensed) because the dashboard must stay one offline file. Embedding a subset open-licence condensed face as base64 is the upgrade once the team supplies a font file it may redistribute.
