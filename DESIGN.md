---
name: Robot Telemetry
description: Bench dashboard for the CSC01 Pico WH robot, laid out as a split-flap departure board.
colors:
  board: "#121212"
  cell: "#1d1d1d"
  cell-hi: "#262626"
  seam: "#0a0a0a"
  frame: "#2e2e2e"
  ink: "#f2f2f2"
  ink-2: "#b8b8b8"
  ink-3: "#8a8a8a"
  amber: "#ffb000"
  amber-hover: "#ffc340"
  red: "#ff5a4a"
  red-lamp: "#c8352b"
  red-cell: "#3a1512"
  green: "#5fd38a"
  steel: "#8fb3d0"
typography:
  display:
    fontFamily: "DIN Condensed, Bahnschrift, Arial Narrow, Roboto Condensed, Helvetica Neue, sans-serif"
    fontSize: "54px"
    fontWeight: 700
    lineHeight: 1
  headline:
    fontFamily: "DIN Condensed, Bahnschrift, Arial Narrow, Roboto Condensed, Helvetica Neue, sans-serif"
    fontSize: "26px"
    fontWeight: 400
    letterSpacing: "0.06em"
  title:
    fontFamily: "DIN Condensed, Bahnschrift, Arial Narrow, Roboto Condensed, Helvetica Neue, sans-serif"
    fontSize: "22px"
    fontWeight: 700
    letterSpacing: "0.12em"
  body:
    fontFamily: "DIN Condensed, Bahnschrift, Arial Narrow, Roboto Condensed, Helvetica Neue, sans-serif"
    fontSize: "16px"
    fontWeight: 400
    lineHeight: 1.3
    fontFeature: "tnum"
  label:
    fontFamily: "DIN Condensed, Bahnschrift, Arial Narrow, Roboto Condensed, Helvetica Neue, sans-serif"
    fontSize: "13px"
    fontWeight: 700
    letterSpacing: "0.14em"
rounded:
  none: "0px"
  face: "2px"
  lamp: "50%"
spacing:
  seam: "2px"
  cell-y: "14px"
  cell-x: "16px"
  page-phone: "16px"
  page: "24px"
  section: "28px"
components:
  button-send:
    backgroundColor: "{colors.amber}"
    textColor: "{colors.board}"
    rounded: "{rounded.face}"
    padding: "0 28px"
    height: "56px"
  button-send-hover:
    backgroundColor: "{colors.amber-hover}"
  button-send-disabled:
    backgroundColor: "{colors.frame}"
    textColor: "{colors.ink-3}"
  letter-cell:
    backgroundColor: "{colors.cell-hi}"
    textColor: "{colors.ink}"
    typography: "{typography.display}"
    rounded: "{rounded.face}"
    width: "68px"
    height: "80px"
  board-cell:
    backgroundColor: "{colors.cell}"
    textColor: "{colors.ink}"
    typography: "{typography.headline}"
    padding: "14px 16px"
  board-cell-fault-row:
    backgroundColor: "{colors.red-cell}"
  board-header:
    backgroundColor: "{colors.board}"
    textColor: "{colors.ink-3}"
    typography: "{typography.label}"
    padding: "10px 16px 8px"
  panel:
    backgroundColor: "{colors.cell}"
    padding: "14px 16px"
---

# Design System: Robot Telemetry

**Contents**

- [Overview](#overview)
- [Colors](#colors)
- [Typography](#typography)
- [Layout](#layout)
- [Elevation & Depth](#elevation--depth)
- [Shapes](#shapes)
- [Components](#components)
- [Do's and Don'ts](#dos-and-donts)

## Overview

**Creative North Star: "The Concourse Departure Board"**

The robot's state reads like a split-flap departure board: fixed rows per subsystem, fixed columns, and only the cells change. A near-black board carries matte cell faces separated by hairline seams; white condensed capitals sit in those cells, and a lamp at the head of each row tells the state before any number is read. Nothing floats: every reading has a permanent address on the board.

Density is high and deliberate. The board is read standing at a lab bench, half-watching the robot, so type is large, upper-case and tabular, and colour is held back for meaning. Amber means warning, stale or pending; red means stop or fault; green means healthy or installed. Cleared states dim to the third ink rather than disappearing.

The one motion in the system is the flap: when a cell's text changes, only the changed characters turn over, one after another, once.

**Key Characteristics:**

- Dark board, matte cells, 2px seams; no floating tiles.
- Condensed upper-case capitals with tabular figures everywhere.
- Row lamps carry state; colour is semantic, never decorative.
- Cleared and stale states dim instead of vanishing.
- Character-by-character flip on change, removed under reduced motion.

## Colors

A monochrome board lit by three signal lamps (amber, red, green) and one cool chart line.

### Primary

- **Signal Amber** (`amber`): the only actionable colour and the warning lamp. Fills the Send mapping button, marks warning, stale and pending rows and lamps, event timestamps, focus outlines, text selection and the input caret. Lighter **Amber Hover** (`amber-hover`) is the button's hover face only.

### Secondary

- **Stop Red** (`red-lamp`): row and header lamp dots, and the large bold State text in a faulted row (about 3.1:1 on the fault wash, acceptable only at large bold sizes).
- **Fault Text Red** (`red`): every smaller red text (fault cells, red header lamp labels, invalid letters, State text on phones) so it holds 4.5:1.
- **Fault Wash** (`red-cell`): replaces the cell face across an entire faulted row.
- **Healthy Green** (`green`): the OK lamp, an installed mapping, accepted acknowledgements.

### Tertiary

- **Steel** (`steel`): the right-wheel line in the wheel-speed chart, paired with Ink for the left wheel. Used nowhere else.

### Neutral

- **Board** (`board`): page background, table header cells, and the dark split line across each letter cell.
- **Cell Face** (`cell`): every data cell, strip cell and panel.
- **Raised Cell** (`cell-hi`): the letter-entry cells, a step lighter than ordinary faces.
- **Seam** (`seam`): the 2px gutters between cells and the rules between log entries.
- **Frame** (`frame`): 1px outer frame around the strip, board and panels; the header rule; idle lamps; chart grid; disabled button.
- **Ink** (`ink`): primary text and the left-wheel chart line.
- **Ink 2** (`ink-2`): row names, lamp labels, letter-cell captions, legends.
- **Ink 3** (`ink-3`): column headers, section headings, cleared faults, refused events, and every cell while the stream is stale.

### Named Rules

**The Lamp Semantics Rule.** Amber is warning, stale or pending; red is stop or fault; green is healthy or installed. No colour appears for any other reason, and amber is never reused as a decorative accent.

**The Dim, Don't Vanish Rule.** A cleared fault or a stale stream drops to Ink 3; it stays in its cell so the team can see what was there.

**The Two Reds Rule.** Stop Red is for dots and large bold State text only; any red text smaller than that uses Fault Text Red.

## Typography

**Display Font:** DIN Condensed (with Bahnschrift, Arial Narrow, Roboto Condensed, Helvetica Neue, sans-serif)
**Body Font:** the same condensed stack, set semi-condensed with tabular figures.

**Character:** One condensed grotesque face in upper-case capitals, like the printed flaps of a station board. Tabular figures keep changing numbers from jittering in their cells.

**Open item:** the shipped face is a system condensed stack because the dashboard must stay one offline file. It is provisional, not a chosen identity: the upgrade is a subset open-licence condensed face embedded as base64 once the team supplies a font file it may redistribute. Rendering currently varies by laptop.

### Hierarchy

- **Display** (700, 54px, line-height 1; 42px on phones): the four mapping letters only.
- **Headline** (400, 26px, 0.06em, upper-case): board data cells. State cells are bold. Row names and fault cells step down to 20px.
- **Title** (700, 22px, 0.12em, upper-case): the page title and the acknowledgement headline. Button text uses the same voice at 18px.
- **Body** (400, 16px, line-height 1.3): the event log and running text; lamp labels and helper text sit at 13-14px.
- **Label** (700, 13px, 0.14em, upper-case, Ink 3): column headers and section headings (11px headers on phones).

### Named Rules

**The Capitals Rule.** Every board, lamp, button and heading string is set in upper case with positive tracking (0.06-0.14em). Only helper sentences and log text stay in sentence case.

**The Tabular Rule.** Figures are tabular everywhere; a changing number must never shift its neighbours.

## Layout

A single centred column (max 1440px) with 24px page padding. Order top to bottom: header bar (title left, three lamps right, 1px Frame rule beneath); the full-width mapping strip; the subsystem board; then a two-column lower band (chart 3fr, event log 2fr, 24px gap). Section headings sit 28px above their block.

The strip and the board are built the same way: a Seam-coloured ground with 2px gaps, so every cell is separated by a dark gutter and the whole is wrapped in a 1px Frame. The board is a fixed-layout table with set column widths (Subsystem 14%, State 16%, Reading 20%, Fault 22%, Detail takes the rest); cells hold one line and truncate with an ellipsis rather than wrapping.

**Breakpoints.**

- **1100px and below:** the strip becomes two columns, with its title and the acknowledgement spanning full width; the lower band stacks into one column.
- **720px and below:** page padding drops to 16px; the strip becomes one column; letter cells stretch to share the width at 64px tall; the button goes full width; the board drops its Detail column and re-weights the rest (22/27/31/20%) with tighter cells and smaller type.

**The Fixed Address Rule.** Rows and columns do not move or reorder with data; Mission is always the first row. Only the content of cells changes.

## Elevation & Depth

The system is flat: no shadows anywhere. Depth comes from tone alone: Seam gutters below the Board, Cell faces above it, Raised Cells above those. The only dimensional effect is the flap's brief 3D turn.

**The Flat Board Rule.** Depth is gutter and tone, never shadow or blur.

## Shapes

Square-cut. Cells, panels, the strip and the board have no radius; letter cells and the button take a barely-softened 2px corner like a physical flap. The only round forms are the 8-9px lamp dots. Borders are 1px Frame on the outside and 2px Seam gutters inside; each letter cell carries a 2px Frame top edge and a dark 2px Board line across its middle, the split of a flap.

## Components

### Buttons

Solid and singular; there is one primary action.

- **Shape:** 2px corners, 56px tall, 0 28px padding; full width on phones.
- **Primary:** Signal Amber face with Board text, title voice at 18px, 0.1em tracking.
- **Hover / Focus:** hover lightens to Amber Hover; focus shows the global 2px amber outline at 2px offset.
- **Disabled:** Frame face, Ink 3 text, not-allowed cursor. The button stays disabled until the mapping is valid and the robot is stopped.

### Inputs / Fields

- **Style:** each letter is a flap cell: Raised Cell face, no border except a 2px Frame top edge, 2px corners, a Board split line through the middle, centred 54px bold capital, amber caret. A caption (Left, Right, Straight, U-turn) sits above in Ink 2 label type.
- **Focus:** amber outline drawn inside the cell (offset -2px).
- **Error:** the letter and top edge turn Fault Text Red.

### Lamps

- **Style:** an 8-9px dot before the label. Idle is Ink 3 in the header and Frame on board rows; ok is green; warn is amber; bad is Stop Red. Header lamp labels in warn or bad states take amber or Fault Text Red; healthy labels stay Ink 2.

### Subsystem Board (signature)

Fixed six rows (Mission, Line IR, Motion, Obstacle, Terrain, Network) by State, Reading, Detail and Fault. Header cells sit on Board in label type; data cells are Cell faces in headline type. A row's lamp leads its first cell. A faulted row floods every cell with Fault Wash and sets State in bold Stop Red. Fault cells hold one line: active faults in Fault Text Red, faults cleared this run dimmed with "cleared", the full list in the tooltip. When the stream is stale every cell dims to Ink 3.

### Mapping Strip (signature)

One framed strip: title cell with installed-state note, four letter cells, acknowledgement cell (22px headline in green, amber or red by outcome, with an Ink 2 message), then the send cell. All cells share the 2px seam grid.

### Panels and Event Log

- **Panel:** Cell face, 1px Frame, 14px 16px padding, square corners, no shadow.
- **Event log:** a departures list: amber time column (6.5em) and message, entries split by Seam rules, scrolling at about 262px. Accepted entries are green; refused ones are Ink 3 with a strike-through.
- **Chart:** 220px canvas on a panel, Frame grid, Ink 3 axis text, Ink and Steel 2px lines with a matching upper-case legend.

### Flap Motion

When a cell's text changes, only the changed characters flip: each turns from -88deg on the X axis (200px perspective, starting at 0.2 opacity) to flat over 240ms with `cubic-bezier(.16, 1, .3, 1)`, staggered 16ms per changed character. Text longer than 40 characters updates plainly. Under `prefers-reduced-motion: reduce` every update is plain.

## Do's and Don'ts

### Do:

- **Do** give every new reading a fixed cell on the board grid, separated by the 2px Seam gutter inside a 1px Frame.
- **Do** keep amber, red and green to their lamp meanings, and use Fault Text Red for any red text below large bold sizes.
- **Do** dim cleared or stale values to Ink 3 rather than removing them.
- **Do** set board text in upper-case condensed capitals with tabular figures.
- **Do** route every changing cell through the flap, and honour reduced motion with a plain update.
- **Do** style standard buttons and inputs as cells, keeping native elements.

### Don't:

- **Don't** build floating tiles or card grids; the board is one framed grid.
- **Don't** add shadows, blur or gradients for depth; depth is tone and gutter. The letter cell's split line is the one gradient, and it draws a seam.
- **Don't** round corners beyond 2px; only lamp dots are circular.
- **Don't** animate anything other than a changed cell's characters.
- **Don't** treat the system condensed stack as the final face; replace it with an embedded redistributable condensed face when one is supplied.
