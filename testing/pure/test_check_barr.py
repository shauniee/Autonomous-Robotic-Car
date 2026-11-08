"""Coding rule scan against small C snippets; no git, build or robot."""
import sys
import os
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
import check_barr as barr


def messages(text, name="sample.c"):
    return [message for _, message in barr.check_text(name, text)]


CLEAN = """/* goto long double 1.5 if (x) y; */
#include "sample.h"
#define WAIT() do { poll(); } while (0)

typedef struct
{
    uint8_t value;
} sample_t;

static const uint8_t table[2] = { 1u, 2u };
static const char text[] = "goto; if (x) y; {";

int sample_run(int count)
{
    int total = 0;

    for (int i = 0; i < count; ++i)
    {
        total += i;
    }
    do
    {
        total--;
    }
    while (total > 0);
    if (count > 1)
    {
        total = 1;
    }
    else if (count > 0)
    {
        total = 2;
    }
    else
    {
        total = 3;
    }
    switch (total)
    {
        case 1:
            break;
        default:
            break;
    }
    return total + (int)(1.5f * 2.0f);
}
"""

assert messages(CLEAN) == [], messages(CLEAN)

BAD = [
    ("void f(void)\n{\n\tg();\n}\n", "tab found"),
    ("int a;\r\n", "CRLF"),
    ("int a; /*" + "x" * 80 + "*/\n", "columns"),
    ('#include "other.c"\n', ".c file"),
    ('#include "/abs/path.h"\n', "absolute path"),
    ("void f(void) {\n}\n", "own line"),
    ("if (a)\n{\n}\n} else\n{\n}\n", "else must"),
    ("a = 1; b = 2;\n", "one statement"),
    ("if (a) b();\n", "if body"),
    ("if (a &&\n    b)\n    c();\n", "if body"),
    ("for (;;) b();\n", "for body"),
    ("while (a) b();\n", "while body"),
    ("do b(); while (a);\n", "do body"),
    ("if (a)\n{\n}\nelse b();\n", "else body"),
    ("switch (a)\n{\n    case 1:\n        break;\n}\n", "no default"),
    ("typedef struct\n{\n    int a;\n} Motion;\n", "must end in _t"),
    ("typedef void (*Handler)(void);\n", "must end in _t"),
    ("goto done;\n", "goto"),
    ("continue;\n", "continue"),
    ("long a;\n", "fixed-width"),
    ("double a;\n", "double"),
    ("float a = 1.5;\n", "f suffix"),
]
for text, expected in BAD:
    found = messages(text)
    assert any(expected in message for message in found), (text, found)

assert any("guard" in m for m in messages("int a;\n", "sample.h"))
assert not any("guard" in m for m in
               messages("#ifndef A_H\n#define A_H\n#endif\n", "sample.h"))
assert any("snake_case" in m for m in messages("int a;\n", "BadName.c"))

# Line numbers survive multi-line comments and literals.
assert barr.check_text("sample.c", "/* one\ntwo */\ngoto x;\n") == [
    (3, barr.BANNED_WORDS["goto"])]

# BARR-C:2018 statement rules, each with a compliant neighbour.
STATEMENT_RULES = (
    ("while (1)\n{\n}\n", "for (;;)\n{\n}\n", "8.4.c"),
    ("exit(1);\n", "report_exit(1);\n", "8.5.b"),
    ("if (a = b)\n{\n}\n", "if (a == b)\n{\n}\n", "8.2.c"),
    ("if (count == 3u)\n{\n}\n", "if (3u == count)\n{\n}\n", "8.6.a"),
    ("if (ptr->state == STATE_IDLE)\n{\n}\n",
     "if (STATE_IDLE == ptr->state)\n{\n}\n", "8.6.a"),
    ("uint8_t first, second;\n", "uint8_t first;\n", "8.1.a"),
    ("int32_t value : 21;\n", "uint32_t value : 21;\n", "5.3.a"),
)
for bad, good, rule in STATEMENT_RULES:
    assert any(m.startswith(rule) for m in messages(bad)), (rule, bad)
    assert not any(m.startswith(rule) for m in messages(good)), (rule, good)

assert barr.in_scope("app_program/motion/motion.c")
assert not barr.in_scope("kernel/tkernel/timer.c")
assert not barr.in_scope("app_program/motion/README.md")

# The hook remembers each in-scope file once and reports the list only once.
session = f"test-{os.getpid()}"
root = Path(__file__).resolve().parents[2]
target = str((root / "app_program/app_main.c").resolve())
assert barr.take_paths(session) == []
for path in (target, target, str(root / "kernel/tkernel/timer.c")):
    barr.run_record({"session_id": session, "tool_input": {"file_path": path}})
assert barr.take_paths(session) == [target]
assert barr.take_paths(session) == []
assert barr.run_check({"session_id": session}) == 0

print("check_barr: all checks passed")
