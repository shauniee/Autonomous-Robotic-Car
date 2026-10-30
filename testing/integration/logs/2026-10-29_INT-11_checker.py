"""Static documentation checks; run with Python 3 from repository root."""
from pathlib import Path
import re
import subprocess
import urllib.parse

root = Path.cwd()
files = sorted(p for p in root.rglob('*.md') if '.git' not in p.parts)
errors = []
link_count = 0

def headings(text):
    fence = False
    counts = {}
    output = []
    for line in text.splitlines():
        if re.match(r'^\s*(```|~~~)', line):
            fence = not fence
            continue
        if fence:
            continue
        match = re.match(r'^(#{1,6})\s+(.+?)\s*#*$', line)
        if not match:
            continue
        title = match[2]
        plain = re.sub(r'\[([^]]+)\]\([^)]*\)', r'\1', title)
        anchor = re.sub(r'[^\w\- ]', '', re.sub(r'<[^>]+>', '', plain).lower().replace('`', '')).replace(' ', '-')
        number = counts.get(anchor, 0)
        counts[anchor] = number + 1
        output.append((len(match[1]), title, anchor + (f'-{number}' if number else '')))
    return output

for path in files:
    source = path.read_text()
    relative = path.relative_to(root)
    lines = source.splitlines()
    if len(lines) < 3 or lines[2] != '## Contents':
        errors.append(f'{relative}: contents must immediately follow title')
    expected = ['  ' * (depth - 2) + f'- [{title}](#{anchor})'
                for depth, title, anchor in headings(source)
                if depth >= 2 and title != 'Contents']
    contents = source.split('## Contents', 1)[1].lstrip('\n')
    actual = []
    for line in contents.splitlines():
        if re.match(r'^\s*[-*] \[', line):
            actual.append(line)
        elif line.strip():
            break
    if actual != expected:
        errors.append(f'{relative}: contents mismatch')
    without_code = re.sub(r'```[\s\S]*?```', '', source)
    for match in re.finditer(r'\[[^\]\n]*\]\(([^\n]+?)\)', without_code):
        target = match[1].strip().strip('<>')
        if re.match(r'^[a-zA-Z][\w+.-]*:', target):
            continue
        name, _, fragment = target.partition('#')
        destination = (path.parent / urllib.parse.unquote(name)).resolve() if name else path
        link_count += 1
        if not destination.exists():
            errors.append(f'{relative}: missing {target}')
        elif fragment and destination.suffix == '.md':
            available = {anchor for _, _, anchor in headings(destination.read_text())}
            if urllib.parse.unquote(fragment) not in available:
                errors.append(f'{relative}: missing anchor {target}')

preserved = 0
for path in sorted((root / 'testing').rglob('TEST_LOG.md')):
    relative = str(path.relative_to(root))
    # Read the index-to-worktree diff: HEAD tree reads may hydrate unavailable
    # iCloud objects. Require the index to equal HEAD for these records first.
    staged = subprocess.run(['git', 'diff', '--cached', '--quiet', '--', relative])
    if staged.returncode:
        errors.append(f'{relative}: staged baseline differs from HEAD')
        continue
    patch = subprocess.check_output(['git', 'diff', '--no-ext-diff', '--unified=100000', '--', relative], text=True)
    original_lines = []
    in_hunk = False
    for line in patch.splitlines(keepends=True):
        if line.startswith('@@'):
            in_hunk = True
            continue
        if in_hunk and line.startswith((' ', '-')):
            original_lines.append(line[1:])
    original = ''.join(original_lines)
    start = re.search(r'^## (?:1\. )?Test cases$', original, re.M)
    if not start:
        errors.append(f'{relative}: baseline test heading not found')
        continue
    body = original[start.start():]
    # Destination maintenance only; expectations/results must remain byte-identical.
    body = body.replace('../../docs/architecture/MOTION.md#business-logic', '../../docs/architecture.md#motion-command-contract')
    body = body.replace('../../docs/architecture/LINE_TRACKING.md', '../../docs/architecture/subsystem3.md')
    if body not in path.read_text():
        errors.append(f'{relative}: historical body changed')
    else:
        preserved += 1

whitespace = subprocess.run(['git', 'diff', '--check'], capture_output=True, text=True)
if whitespace.returncode:
    errors.append(whitespace.stdout + whitespace.stderr)

print('INT-11: documentation/static validation only')
print(f'Markdown files: {len(files)}')
print(f'Local links and anchors checked: {link_count}')
print(f'Historical TEST_LOG bodies preserved: {preserved}')
print('Contents: checked against actual headings')
print(f'git diff --check: {"PASS" if whitespace.returncode == 0 else "FAIL"}')
print(f'Result: {"PASS" if not errors else "FAIL"}')
for error in errors:
    print(error)
raise SystemExit(bool(errors))
