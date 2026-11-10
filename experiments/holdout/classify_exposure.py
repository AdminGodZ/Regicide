#!/usr/bin/env python3
"""Classify complete Regicide match-runner PDNs without searching.

Requires a compiled exposure_helper. Run with --help. Assumes book enabled, deterministic
margin zero, no searchmoves/infinite mode, no extra custom avoid list, and the given DB
coverage. Opening tags must name the dealt moves. Unknown DB/history never means clean.
Every record is replayed by BOTH parsers before any positive classification is emitted.
The raw-entry exposure column is conservative; first_probe_change_ply identifies differing
deterministic Book::probe results. A differing score alone may change the returned comment.
"""
import argparse
import re
import subprocess
import sys
from pathlib import Path

def games(path):
    tags, body = {}, []
    for line in Path(path).read_text().splitlines():
        m = re.fullmatch(r'\s*\[(\w+)\s+"(.*)"\]\s*', line)
        if m:
            if body:
                yield tags, " ".join(body)
                tags, body = {}, []
            tags[m[1]] = m[2]
        elif line.strip():
            body.append(line.strip())
    if body:
        yield tags, " ".join(body)

def moves(body):
    body = re.sub(r'\{[^{}]*\}', ' ', body)
    if any(c in body for c in '{}();'):
        raise ValueError('unsupported PDN comment/variation syntax')
    result = []
    for token in body.split():
        token = re.sub(r'^\d+\.{1,3}', '', token)
        if not token or token in ('1-0', '0-1', '1/2-1/2', '*'):
            continue
        if not re.fullmatch(r'\d+(?:[-x:]\d+)+', token):
            raise ValueError('unsupported PDN token: ' + token)
        result.append(token)
    return result

def main():
    p = argparse.ArgumentParser(description=__doc__)
    for x in ('helper', 'old-book', 'new-book'):
        p.add_argument('--' + x, required=True)
    group = p.add_mutually_exclusive_group(required=True)
    group.add_argument('--our-name', help='exact Black/White player tag')
    group.add_argument('--our-color', choices=['B', 'W'])
    p.add_argument('--opening-plies', type=int, help='override tag inference; must be declared explicitly if Opening tag absent')
    p.add_argument('--egdb')
    p.add_argument('--db-pieces', type=int, default=8)
    p.add_argument('--no-progress', type=int, default=40, help='runner no-progress rule, in moves')
    p.add_argument('--max-plies', type=int, default=1000, help='runner limit, excluding dealt plies')
    p.add_argument('--book-off', action='store_true')
    p.add_argument('--played-book', choices=['old', 'new'], help='also check played moves against this arm at changed roots')
    p.add_argument('pdn', nargs='+')
    a = p.parse_args()
    rows = []
    for path in a.pdn:
        for i, (tags, body) in enumerate(games(path), 1):
            if tags.get('Result') not in ('1-0', '0-1', '1/2-1/2'):
                raise ValueError(f'{path} game {i}: unfinished result')
            seq = moves(body)
            opening = tags.get('Opening')
            if a.opening_plies is not None:
                dealt = a.opening_plies
            elif opening is None:
                raise ValueError('Opening tag missing; supply --opening-plies')
            else:
                prefix = moves(opening.rsplit(': ', 1)[1]) if ': ' in opening else []
                if seq[:len(prefix)] != prefix:
                    raise ValueError('Opening tag disagrees with movetext')
                dealt = len(prefix)
            side = a.our_color
            if side is None:
                sides = [c for c, tag in [('B', 'Black'), ('W', 'White')] if tags.get(tag) == a.our_name]
                if len(sides) != 1:
                    raise ValueError(f'{path} game {i}: our name must identify exactly one side')
                side = sides[0]
            # The runner sends the complete dealt + played history from this fresh FEN/startpos.
            rows.append('\t'.join([f'{path}:{i}', side, str(dealt), '0', tags.get('FEN', '-'), ' '.join(seq)]))
    cmd = [a.helper, a.old_book, a.new_book, '--db-pieces', str(a.db_pieces),
           '--no-progress', str(a.no_progress), '--max-plies', str(a.max_plies)]
    if a.egdb:
        cmd += ['--egdb', a.egdb]
    if a.book_off:
        cmd += ['--book-off']
    if a.played_book:
        cmd += ['--played-book', a.played_book]
    subprocess.run(cmd, input='\n'.join(rows) + '\n', text=True, check=True)

if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError, subprocess.CalledProcessError) as exc:
        sys.exit(str(exc))
