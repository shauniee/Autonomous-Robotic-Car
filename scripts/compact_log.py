#!/usr/bin/env python3
"""Make monitor and serial captures short enough to read.

- WHEEL and LINE frames print as short key=value lines instead of JSON.
- A periodic line (FOLDED types) identical to the previous line of its type,
  apart from its time or sequence number, is not printed. Events, scans and
  barcode runs are never folded: each one is a separate occurrence. When that type next changes (or the
  capture ends) one line says how many were folded:
      WHEEL repeated x1240 to t=280170
  Nothing that changes is lost; only the exact repeats are.

Usage: compact_log.py IN [OUT]   (.gz in or out is handled; OUT defaults to stdout)
"""
import gzip
import json
import re
import sys

VOLATILE = re.compile(r"\b(?:t|seq|age_ms)=\d+|#\d+|^\d{2}:\d{2}:\d{2}(?:\.\d+)? ")
TIME = re.compile(r"\bt=(\d+)")
FOLDED = {"WHEEL", "LINE", "TELEMETRY", "HUMP", "STATUS", "[IR]", "[TIMING]", "wifi:", "data:"}
LINE_FLAGS = ("left_black", "right_black", "in_broad", "junction_open",
              "junction_qualified", "entry_curved", "junction_locked", "barcode_level")


def _pair(a, b):
    return f"{'-' if a is None else a}/{'-' if b is None else b}"


def wheel_text(trace):
    text = (f"WHEEL t={trace['sampled_ms']} target={_pair(trace['target_left'], trace['target_right'])} "
            f"speed={_pair(trace['measured_left'], trace['measured_right'])} "
            f"duty={_pair(trace['duty_left'], trace['duty_right'])}")
    if trace.get("inhibited"):
        text += " inhibited"
    if not trace.get("output_ok", True):
        text += " output-failed"
    return text


def line_text(trace):
    flags = [key for key in LINE_FLAGS if trace.get(key)]
    skew = trace.get("onset_skew_mm")
    return (f"LINE t={trace['sampled_ms']} {trace['mission']}/{trace['phase']} "
            f"counts={trace['left']}/{trace['right']} steer={trace['steer_q']} law={trace['law_q']} "
            f"cmd={trace['command']}:{trace['command_speed']}:{trace['command_steer_q']} "
            f"edges={trace['barcode_edges']} skew={'-' if skew is None else skew}"
            + (" " + ",".join(flags) if flags else ""))


def compact_line(line):
    """One capture line in its short form; anything unrecognised is unchanged."""
    for tag, render in (("WHEEL ", wheel_text), ("LINE ", line_text)):
        if line.startswith(tag + "{"):
            try:
                return render(json.loads(line[len(tag):]))
            except (ValueError, KeyError, TypeError):
                return line
    return line


def line_type(line):
    return line.split(" ", 1)[0] if line else ""


class Folder:
    """Drops exact repeats per line type and reports how many it dropped."""

    def __init__(self):
        self.last = {}      # type -> normalised text of the last printed line
        self.repeats = {}   # type -> (count, last time seen)

    def _summary(self, kind):
        count, last_time = self.repeats.pop(kind, (0, None))
        if not count:
            return []
        return [f"{kind} repeated x{count}" + (f" to t={last_time}" if last_time else "")]

    def feed(self, line):
        line = compact_line(line.rstrip("\r\n"))
        kind = line_type(line)
        if kind not in FOLDED:
            return [line]
        key = VOLATILE.sub("", line)
        if self.last.get(kind) == key:
            count, _ = self.repeats.get(kind, (0, None))
            time = TIME.search(line)
            self.repeats[kind] = (count + 1, time.group(1) if time else None)
            return []
        self.last[kind] = key
        return self._summary(kind) + [line]

    def flush(self):
        out = []
        for kind in list(self.repeats):
            out += self._summary(kind)
        return out


def compact_lines(lines):
    folder = Folder()
    for line in lines:
        yield from folder.feed(line)
    yield from folder.flush()


def _open(path, mode):
    if path == "-":
        return sys.stdin if "r" in mode else sys.stdout
    if path.endswith(".gz"):
        return gzip.open(path, mode + "t", encoding="utf-8", errors="replace")
    return open(path, mode, encoding="utf-8", errors="replace")


def main(argv):
    if len(argv) not in (2, 3):
        print(__doc__.strip(), file=sys.stderr)
        return 2
    with _open(argv[1], "r") as source:
        lines = list(compact_lines(source))
    with _open(argv[2] if len(argv) == 3 else "-", "w") as target:
        for line in lines:
            target.write(line + "\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
