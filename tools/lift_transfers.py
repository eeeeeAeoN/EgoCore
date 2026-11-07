#!/usr/bin/env python3
"""Harvest `<Class>::Transfer` bodies with the decomp engine.

    ./target/release/decomp-cli lift '<Class>::Transfer'

Run this once, then feed the output directory to tools/gen_transfer.py. The raw
lifts are derived from the proprietary PDB and are not committed.
"""
import argparse
import glob
import os
import re
import subprocess
import sys


def classes_with_transfer(tree):
    out = []
    ns = []
    stack = []
    for path in sorted(glob.glob(os.path.join(tree, '**', '*.hpp'), recursive=True)):
        lines = open(path, encoding='utf-8', errors='replace').read().splitlines()
        i = 0
        while i < len(lines):
            line = lines[i].strip()
            i += 1
            if not line or line.startswith('#') or line.startswith('//'):
                continue
            m = re.match(r'namespace\s+([A-Za-z_]\w*)\s*\{$', line)
            if m:
                ns.append(m.group(1))
                stack.append(('ns', m.group(1)))
                continue
            if line.startswith('}'):
                if stack:
                    kind = stack.pop()
                    if kind[0] == 'ns':
                        ns.pop()
                continue
            m = re.match(r'^(?:template\s*<.*>\s*)?(?:class|struct)\s+([A-Za-z_]\w*)\s*(?::\s*[^{]*)?$', line)
            if m and i < len(lines) and lines[i].strip() == '{':
                i += 1
                name = m.group(1)
                parent = next((f[1] for f in reversed(stack) if f[0] == 'class'), None)
                if parent:
                    q = parent[0] + '::' + name
                else:
                    q = '::'.join(ns + [name]) if ns else name
                cls = {'q': q, 'transfer': False}
                stack.append(('class', (q, cls)))
                if q not in [c['q'] for c in out]:
                    out.append(cls)
                continue
            if stack and stack[-1][0] == 'class':
                if 'Transfer(CPersistContext' in line:
                    stack[-1][1][1]['transfer'] = True
    return [c['q'] for c in out if c['transfer']]


def sanitize(name):
    return re.sub(r'[^A-Za-z0-9_.-]', '_', name)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--tree', required=True, help='decomp-cli tree output')
    ap.add_argument('--decomp-cli', required=True, help='path to decomp-cli')
    ap.add_argument('--out', default='/tmp/fable-transfer-lifts')
    ap.add_argument('--limit', type=int, default=0)
    args = ap.parse_args()

    os.makedirs(args.out, exist_ok=True)
    names = classes_with_transfer(args.tree)
    if args.limit:
        names = names[:args.limit]
    print(f'{len(names)} classes with Transfer', file=sys.stderr)

    log = open(os.path.join(args.out, 'log.txt'), 'a', buffering=1)
    for index, cls in enumerate(names, 1):
        path = os.path.join(args.out, sanitize(cls) + '.txt')
        if os.path.exists(path) and os.path.getsize(path) > 0:
            continue
        print(f'[{index}/{len(names)}] {cls}', file=sys.stderr)
        try:
            result = subprocess.run(
                [args.decomp_cli, 'lift', f'{cls}::Transfer'],
                capture_output=True, text=True, timeout=180)
            text = result.stdout + result.stderr
        except subprocess.TimeoutExpired:
            text = 'timeout\n'
            result = None
        open(path, 'w').write(text)
        code = result.returncode if result is not None else 124
        log.write(f'{cls} {code}\n')


if __name__ == '__main__':
    main()
