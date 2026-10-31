# Design system: Robot Telemetry

## Contents

- [Direction](#direction)
- [Colour and typography](#colour-and-typography)
- [Composition](#composition)
- [States and interaction](#states-and-interaction)
- [Verification](#verification)

## Direction

A compact bench ledger: stable addresses for Mission and every subsystem,
with sensor detail and custom barcode configuration visible together.
Retain the existing dark, square-cut board and semantic signal lamps;
replace the tall departure-board composition with a single desktop overview.
Product truth and offline constraints belong to [PRODUCT.md](PRODUCT.md).
The route's mode and viewport contract live in the
[surface brief](.impeccable/surfaces/scripts-dashboard-html.md).

## Colour and typography

Page #121212, data surfaces #1d1d1d, inputs #262626, rules #383838.
Primary text #f2f2f2, secondary #b8b8b8, muted #999999.
Amber #ffb000 denotes pending, warning and the primary mapping action;
green #5fd38a denotes healthy or accepted; red #ff7567 denotes stop/fault;
fault rows use #3a1512. Steel #8fb3d0 distinguishes the right-wheel trace.
No shadows, decorative gradients or raised cards. Corners are square except
2px inputs/buttons and round status lamps.

Use the offline system sans for data, labels and controls, with tabular
figures. Keep the incumbent condensed face stack for the page title, current
Mission state, state strip and mapping letters. Body/data 12–14px, section
labels 13px, title 22px, current state 25px, editable letters 32px. Secondary
sensor explanations use 11px. Preserve native focus/selection/caret behaviour
with amber selection and focus outlines. Focus must remain visible.

## Composition

At 1440 × 900, show the complete dashboard without document scrolling:
header status lamps; full-width Mission state strip; two-column workspace;
chart/events lower band; local-control reminder.

The workspace gives remaining width to telemetry and 350px to configuration,
with a 16px gutter. Telemetry contains the five-row subsystem table and four
sensor columns. Configuration contains mapping inputs, acknowledgement and
Send mapping, followed by the existing junction-test controls. Keep Mission state and faults in the top strip; order subsystem rows S1–S5.
Motion Reading is the encoder-derived direction (straight labelled Forward);
Detail is left/right speed. S3/S4 Detail shows Calibrated when usable-line or
valid-hump evidence confirms calibration, Fault / not calibrated for reported
owner faults, and Calibration unknown otherwise. S5 retains dimensions.
Telemetry cannot distinguish a U-turn from a left/right turn. Table faults wrap; never hide
active faults behind ellipses. Event history scrolls within its own panel.

Use 24px horizontal page padding, 12px main gaps and 8–16px component padding.
Above 1100px and below 800px high, reduce vertical padding/chart height.
Below 1100px the sensor ledger becomes two columns; below 800px stack the
workspace and lower band, preserving natural page scrolling and keyboard
access. Small windows are a fallback, not the one-screen desktop target.

## States and interaction

Mission states are a vocabulary, not sequential progress. Highlight only the
reported state with a border and `aria-current`; no completion marks or arrows.
Unknown state codes stay visible in the current-state readout. STOPPED and
active faults use red plus text. No data starts with explicit waiting copy;
stale readings stay in place and dim, with stream-age warning text.

Sensor zero is distinct from unavailable data. Retained hump maximum and
completed obstacle dimensions remain labelled; raw IR/IMU/servo data is not
implied. Update readings atomically without character-flip animation.

Four native letter inputs map Left, Right, Straight and U-turn in that order.
Input letters are drafts until an accepted ACK; telemetry only reports whether
a mapping exists, not its four letters. Duplicate, incomplete, offline, stale
and moving states disable Send with an explanation. ACK status shows accepted,
refused or timed-out outcome. Local start/stop controls remain labelled.
Junction tests retain the existing live FOLLOW_LINE gate and one-shot behaviour.

## Verification

Review all content at 1440 × 900, including faults, stale and empty streams;
also check a compact desktop and narrow window. Exercise real mapping and
junction UI logic using synthetic fixtures only. Evidence and exact source
identity belong in [testing](testing/README.md), not this design guide.
