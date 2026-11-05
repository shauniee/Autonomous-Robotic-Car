#!/usr/bin/env python3
"""Check app_program C code against the mechanical rules of BARR-C:2018.

Rule numbers refer to that standard; docs/team.md section 5 lists what the
project adopted. This is a text scan, not a compliance certification, and it
cannot judge the review checklist (units, ownership, synchronization, bounds).

    check_barr.py             changed lines of files that differ from HEAD
    check_barr.py --base REF  changed lines since the merge base with REF (CI)
    check_barr.py --all       every line of every tracked app_program C file
    check_barr.py --summary   count every finding by rule; always exits 0
    check_barr.py FILE...     every line of the named files
    check_barr.py --hook-record   Claude Code hook: remember an edited file
    check_barr.py --hook-check    Claude Code hook: check the remembered files
"""
import json
import os
import re
import subprocess
import sys
import tempfile

MAX_COLUMNS = 80
MAX_HOOK_FINDINGS = 30
BANNED_WORDS = {
    "goto": "goto is banned",
    "continue": "continue is banned",
    "auto": "auto is banned",
    "register": "register is banned",
    "short": "use a fixed-width type, not short",
    "long": "use a fixed-width type, not long",
    "double": "use float with a justification, or fixed point, not double",
}


def blank_comments_and_strings(text):
    """Blank comments and literal contents, keeping every offset and newline."""
    out = list(text)
    i = 0
    n = len(text)
    while i < n:
        if text.startswith("//", i):
            end = text.find("\n", i)
            end = n if end < 0 else end
        elif text.startswith("/*", i):
            end = text.find("*/", i + 2)
            end = n if end < 0 else end + 2
        elif text[i] in "\"'":
            # Keep the quotes so a blanked literal still counts as code.
            quote = text[i]
            end = i + 1
            while end < n and text[end] not in (quote, "\n"):
                end += 2 if text[end] == "\\" else 1
            for j in range(i + 1, min(end, n)):
                out[j] = " "
            i = end + 1
            continue
        else:
            i += 1
            continue
        for j in range(i, end):
            if out[j] != "\n":
                out[j] = " "
        i = end
    return "".join(out)


def blank_preprocessor(code):
    """Blank directives so macro bodies skip the structural checks."""
    lines = code.split("\n")
    in_directive = False
    for index, line in enumerate(lines):
        if in_directive or line.lstrip().startswith("#"):
            in_directive = line.rstrip().endswith("\\")
            lines[index] = " " * len(line)
    return "\n".join(lines)


def matching(code, start, opening, closing):
    """Return the offset of the bracket closing code[start], or -1."""
    depth = 0
    for i in range(start, len(code)):
        if code[i] == opening:
            depth += 1
        elif code[i] == closing:
            depth -= 1
            if depth == 0:
                return i
    return -1


def next_code_char(code, start):
    """Return the offset of the first non-space at or after start, or -1."""
    match = re.compile(r"\S").search(code, start)
    return match.start() if match else -1


def check_structure(code):
    """Yield (offset, message) for rules that can span several lines."""
    for match in re.finditer(r"\b(if|for|while|switch)\b\s*\(", code):
        close = matching(code, match.end() - 1, "(", ")")
        after = next_code_char(code, close + 1) if close >= 0 else -1
        if after < 0:
            continue
        if code[after] == "{":
            if match.group(1) == "switch":
                body_end = matching(code, after, "{", "}")
                body = code[after:body_end if body_end >= 0 else len(code)]
                if not re.search(r"\bdefault\s*:", body):
                    yield match.start(), "switch has no default"
        elif not (match.group(1) == "while" and code[after] == ";"):
            yield match.start(), f"{match.group(1)} body needs braces"
    for match in re.finditer(r"\belse\b", code):
        after = next_code_char(code, match.end())
        if after >= 0 and code[after] != "{" \
                and not code.startswith("if", after):
            yield match.start(), "else body needs braces"
    for match in re.finditer(r"\bdo\b", code):
        after = next_code_char(code, match.end())
        if after >= 0 and code[after] != "{":
            yield match.start(), "do body needs braces"
    for match in re.finditer(r"\btypedef\b", code):
        end = code.find(";", match.end())
        brace = code.find("{", match.end())
        if brace >= 0 and (end < 0 or brace < end):
            close = matching(code, brace, "{", "}")
            end = code.find(";", close) if close >= 0 else -1
        if end < 0:
            continue
        declaration = code[match.end():end]
        pointer = re.search(r"\(\s*\*\s*(\w+)\s*\)", declaration)
        names = re.findall(r"\w+", declaration)
        name = pointer.group(1) if pointer else (names[-1] if names else "")
        array = re.search(r"(\w+)\s*\[[^\]]*\]\s*$", declaration)
        name = array.group(1) if array and not pointer else name
        if name and not name.endswith("_t"):
            yield match.start(), f"type name {name} must end in _t"


def check_line(raw, code, is_directive):
    """Yield messages for one line, given raw text and blanked code."""
    if "\t" in raw:
        yield "tab found, indent with four spaces"
    if raw.endswith("\r"):
        yield "CRLF line ending, use LF"
    if len(raw.rstrip("\r")) > MAX_COLUMNS:
        yield f"line is {len(raw.rstrip())} columns, limit is {MAX_COLUMNS}"
    if is_directive:
        if re.match(r'\s*#\s*include\s*"(/|.*\.c")', raw):
            yield "never include a .c file or an absolute path"
        return
    stripped = code.strip()
    if stripped.endswith("{") and stripped != "{" \
            and not re.search(r"(=|,|\(|\{)\s*\{$", stripped):
        yield "opening brace must be on its own line"
    if re.match(r"\}\s*else\b", stripped):
        yield "else must start its own line"
    depth = 0
    statements = 0
    for char in stripped:
        depth += char in "(["
        depth -= char in ")]"
        statements += char == ";" and depth == 0
    if statements > 1:
        yield "one statement per line"
    for word in re.findall(r"\b[a-z]+\b", stripped):
        if word in BANNED_WORDS:
            yield BANNED_WORDS[word]
    if re.search(r"(?<![\w.])\d+\.\d*(?:[eE][+-]?\d+)?(?![\w.])", stripped):
        yield "float literal needs an f suffix"
    yield from check_statement(stripped)


def check_statement(stripped):
    """Yield BARR-C:2018 statement rules a single line can show."""
    if re.search(r"\bwhile\s*\(\s*(1|true)\s*\)", stripped):
        yield "8.4.c write an infinite loop as for (;;)"
    if re.search(r"\b(abort|exit|setjmp|longjmp)\s*\(", stripped):
        yield "8.5.b abort, exit, setjmp and longjmp are banned"
    if re.search(r"\b(if|while)\s*\((?:[^=!<>()]|\([^()]*\))*[^=!<>]=[^=]",
                 stripped):
        yield "8.2.c no assignment inside a test"
    if re.search(r"\b[a-z_]\w*(?:(?:->|\.)\w+|\[\w+\])*\s*==\s*"
                 r"(?:\d|NULL\b|true\b|false\b|[A-Z][A-Z0-9_]+\b)", stripped):
        yield "8.6.a put the constant on the left of =="
    if re.match(r"(?:(?:const|static|volatile)\s+)*"
                r"(?:u?int(?:8|16|32|64)_t|float|bool|char|unsigned|int)\s+"
                r"\**\w+(?:\s*=[^,;]*)?\s*,\s*\**\w+[^()]*;$", stripped):
        yield "8.1.a declare one variable per declaration"
    if re.match(r"(?:const\s+)?int(?:8|16|32|64)_t\s+\w+\s*:\s*\d+\s*;",
                stripped):
        yield "5.3.a no signed bit-fields"


def check_text(path, text):
    """Return sorted (line, message) findings for one file's text."""
    findings = set()
    name = os.path.basename(path)
    if not re.fullmatch(r"[a-z0-9_]+\.[ch]", name):
        findings.add((1, "file name must be lowercase snake_case"))
    code = blank_comments_and_strings(text)
    structural = blank_preprocessor(code)
    if name.endswith(".h") and not re.search(
            r"^\s*#\s*(ifndef|pragma\s+once)\b", code, re.M):
        findings.add((1, "header has no include guard"))
    for offset, message in check_structure(structural):
        findings.add((text.count("\n", 0, offset) + 1, message))
    code_lines = code.split("\n")
    structural_lines = structural.split("\n")
    for index, raw in enumerate(text.split("\n")):
        # Code that blanking removed was part of a directive or macro body.
        is_directive = bool(code_lines[index].strip()) \
            and not structural_lines[index].strip()
        for message in check_line(raw, structural_lines[index], is_directive):
            findings.add((index + 1, message))
    return sorted(findings)


def git(root, *args):
    """Run git in root and return the completed process."""
    return subprocess.run(["git", "-C", root, *args], capture_output=True,
                          text=True, errors="replace")


def changed_lines(root, relative, base="HEAD"):
    """Return the set of lines differing from base, or None for a new file."""
    if git(root, "ls-files", "--error-unmatch", "--", relative).returncode:
        return None
    if git(root, "cat-file", "-e", f"{base}:{relative}").returncode:
        return None
    lines = set()
    diff = git(root, "diff", "-U0", base, "--", relative).stdout
    for match in re.finditer(r"^@@ -\S+ \+(\d+)(?:,(\d+))? @@", diff, re.M):
        start = int(match.group(1))
        lines.update(range(start, start + int(match.group(2) or 1)))
    return lines


def in_scope(relative):
    """Application C code only; the imported kernel is not reformatted."""
    return relative.startswith("app_program/") \
        and relative.endswith((".c", ".h"))


def report(root, relative, only_changed, base="HEAD"):
    """Return printable findings for one file."""
    path = os.path.join(root, relative)
    with open(path, encoding="utf-8", errors="replace", newline="") as source:
        findings = check_text(relative, source.read())
    wanted = changed_lines(root, relative, base) if only_changed else None
    return [f"{relative}:{line}: {message}" for line, message in findings
            if wanted is None or line in wanted]


def repo_root(directory):
    """Return the enclosing git top level, or None outside a repository."""
    result = git(directory, "rev-parse", "--show-toplevel")
    return result.stdout.strip() if result.returncode == 0 else None


def state_path(session_id):
    """Return the file listing what this Claude session has edited."""
    safe_id = re.sub(r"[^\w-]", "_", session_id)
    return os.path.join(tempfile.gettempdir(), f"check_barr_{safe_id}.txt")


def locate(path):
    """Return (root, relative) for an in-scope existing file, else None."""
    path = os.path.realpath(path)
    if not os.path.isfile(path):
        return None
    root = repo_root(os.path.dirname(path))
    if root is None:
        return None
    relative = os.path.relpath(path, os.path.realpath(root))
    return (root, relative) if in_scope(relative) else None


def take_paths(session_id):
    """Return the recorded paths in order and forget them."""
    state = state_path(session_id)
    try:
        with open(state, encoding="utf-8") as source:
            paths = list(dict.fromkeys(source.read().splitlines()))
    except FileNotFoundError:
        return []
    os.remove(state)
    return paths


def run_record(payload):
    """Remember an edited file; checking waits until the edits are done."""
    path = payload.get("tool_input", {}).get("file_path", "")
    if locate(path) is not None:
        with open(state_path(payload.get("session_id", "")), "a",
                  encoding="utf-8") as state:
            state.write(os.path.realpath(path) + "\n")
    return 0


def run_check(payload):
    """Tell Claude about violations before it reports its changes."""
    lines = []
    # Each recording is reported once, so unfixable lines cannot trap Claude.
    for path in take_paths(payload.get("session_id", "")):
        located = locate(path)
        if located is not None:
            lines += report(located[0], located[1], True)
    if not lines:
        return 0
    print("Coding rule violations on changed lines (docs/team.md section 5).",
          "Fix them or tell the user why not, before replying:",
          file=sys.stderr)
    print("\n".join(lines[:MAX_HOOK_FINDINGS]), file=sys.stderr)
    if len(lines) > MAX_HOOK_FINDINGS:
        print(f"... and {len(lines) - MAX_HOOK_FINDINGS} more",
              file=sys.stderr)
    # Exit 2 blocks the wrap-up or the stop and hands stderr back to Claude.
    return 2


def summary(root):
    """Print every finding counted by rule, for the legacy backlog."""
    counts = {}
    for relative in git(root, "ls-files", "app_program").stdout.split():
        if in_scope(relative):
            for line in report(root, relative, False):
                rule = re.sub(r"\d+", "N", line.split(": ", 1)[1])
                counts[rule] = counts.get(rule, 0) + 1
    for rule, count in sorted(counts.items(), key=lambda item: -item[1]):
        print(f"{count:6d}  {rule}")
    print(f"{sum(counts.values()):6d}  total, legacy code is converted as it "
          "is changed (BARR-C:2018 enforcement section)")
    return 0


def main(argv):
    if argv == ["--hook-record"]:
        return run_record(json.load(sys.stdin))
    if argv == ["--hook-check"]:
        return run_check(json.load(sys.stdin))
    root = repo_root(os.getcwd())
    if root is None:
        print("check_barr.py: not inside a git repository", file=sys.stderr)
        return 2
    base = "HEAD"
    if argv == ["--summary"]:
        return summary(root)
    if len(argv) == 2 and argv[0] == "--base":
        merge = git(root, "merge-base", argv[1], "HEAD")
        if merge.returncode:
            print(f"check_barr.py: no merge base with {argv[1]}",
                  file=sys.stderr)
            return 2
        base = merge.stdout.strip()
        files = git(root, "diff", "--name-only", "--diff-filter=d",
                    base).stdout.split()
        only_changed = True
    elif argv == ["--all"]:
        files = git(root, "ls-files", "app_program").stdout.split()
        only_changed = False
    elif argv:
        files = [os.path.relpath(os.path.realpath(name),
                                 os.path.realpath(root)) for name in argv]
        only_changed = False
    else:
        files = git(root, "diff", "--name-only", "--diff-filter=d",
                    "HEAD").stdout.split()
        files += git(root, "ls-files", "--others", "--exclude-standard",
                     "app_program").stdout.split()
        only_changed = True
    lines = []
    for relative in files:
        if in_scope(relative):
            lines += report(root, relative, only_changed, base)
    if lines:
        print("\n".join(lines))
    print(f"{len(lines)} finding(s)", file=sys.stderr)
    return 1 if lines else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
