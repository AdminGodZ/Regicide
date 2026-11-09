#!/usr/bin/env python3
"""Ponder-test analysis of tools/match result dirs (results.csv + games.pdn with --pdn-comments).
  analyze.py "NAME=run1,run2" ...   (run = dir under ~/regicide-match/results)
Per group: W/D/L, pentanomial pairs and Elo with 95% CI (per-pair mean score, normal approximation, delta method, as
jobs/tm/pool.py), Regicide's losses by ballot and colour, ponder hits/misses, and over Regicide's searched moves
(info with depth >= 1): median depth, median kN/s, median nodes, median time; also split hit / not hit.
With several groups, every group minus the first: Elo difference with a 95% CI (independent samples)."""
import csv, math, os, re, sys, collections, statistics as S

R = os.path.expanduser('~/regicide-match/results')


def elo(s):
    return -400 * math.log10(1 / s - 1)


def pdn_games(path):
    text = open(path, encoding='utf-8', errors='replace').read()
    for chunk in re.split(r'\n(?=\[Event )', text):
        if '[' not in chunk:
            continue
        hdr = dict(re.findall(r'\[(\w+) "([^"]*)"\]', chunk))
        body = re.sub(r'\[[^\]]*\]\s*', '', chunk)
        yield hdr, body


def reg_infos(hdr, body):
    """(comment text) for each Regicide move that has a comment."""
    reg_black = hdr.get('Black', '').startswith('Regicide')
    ply = -1
    out = []
    for tok in re.finditer(r'\{([^}]*)\}|(\d+)\.(?!\d)|([0-9]+[-x][0-9x-]+)|(1-0|0-1|1/2-1/2|\*)', body):
        if tok.group(3):
            ply += 1
        elif tok.group(1) is not None and ply >= 0:
            mover_black = ply % 2 == 0
            if mover_black == reg_black:
                out.append(' '.join(tok.group(1).split()))
    return out


def load(runs):
    W = D = L = 0
    penta = [0] * 5
    pairs = []
    losses = collections.Counter()
    loss_col = collections.Counter()
    hits = misses = hits50 = misses50 = 0
    allm, hitm, nohit, early, ehit, enohit = [], [], [], [], [], []
    for run in runs:
        rows = list(csv.DictReader(open(f'{R}/{run}/results.csv')))
        byp = collections.defaultdict(list)
        for r in rows:
            s = float(r['score_a'])
            byp[(run, r['pair'])].append(s)
            W += s == 1; D += s == 0.5; L += s == 0
            if s == 0:
                col = 'black' if r['black'].startswith('Regicide') else 'white'
                losses[(r['opening'], col)] += 1
                loss_col[col] += 1
        for k, v in byp.items():
            if len(v) == 2:
                t = sum(v)
                penta[int(round(t * 2))] += 1
                pairs.append(t / 2)
        for hdr, body in pdn_games(f'{R}/{run}/games.pdn'):
            k = 0
            for c in reg_infos(hdr, body):
                hit = 'ponder hit' in c
                hits += hit
                misses += 'ponder miss' in c
                pm = re.search(r'ponder (hit|miss) (\d+) ms', c)
                if pm and int(pm.group(2)) >= 50:  # the opponent took at least 50 ms over its reply
                    if hit: hits50 += 1
                    else: misses50 += 1
                m = re.search(r'([\d.]+)s depth (\d+)/(\d+) score (-?\d+) nodes (\d+) (\d+) kN/s', c)
                # searched moves: not forced (depth 1), not decided at the root by the database (depth 127), and
                # not book moves (depth 0)
                if not m or not 2 <= int(m.group(2)) < 127 or int(m.group(5)) < 1000:
                    continue
                rec = (int(m.group(2)), int(m.group(6)), int(m.group(5)), float(m.group(1)))
                allm.append(rec)
                (hitm if hit else nohit).append(rec)
                k += 1
                if k <= 25:
                    early.append(rec)
                    (ehit if hit else enohit).append(rec)
    return dict(W=W, D=D, L=L, penta=penta, pairs=pairs, losses=losses, loss_col=loss_col, hits=hits,
                hits50=hits50, misses50=misses50,
                misses=misses, allm=allm, hitm=hitm, nohit=nohit, early=early, ehit=ehit,
                enohit=enohit)


def stats(pairs):
    n = len(pairs); m = sum(pairs) / n
    var = sum((x - m) ** 2 for x in pairs) / (n - 1); se = math.sqrt(var / n)
    if m <= 0 or m >= 1:
        return m, se, float('nan'), float('nan'), float('nan')
    slope = 400 / math.log(10) / (m * (1 - m))
    return m, se, elo(m), 1.96 * se * slope, slope


def med(recs, i):
    return S.median(r[i] for r in recs) if recs else float('nan')


groups = []
for arg in sys.argv[1:]:
    name, runs = arg.split('=', 1)
    runs = runs.split(',')
    g = load(runs)
    m, se, e, ci, slope = stats(g['pairs'])
    groups.append((name, m, se, e, slope))
    print(f'== {name}: {", ".join(runs)}')
    print(f'   games {g["W"] + g["D"] + g["L"]}: W/D/L {g["W"]}/{g["D"]}/{g["L"]}; pairs {len(g["pairs"])}, '
          f'pentanomial [LL LD DD/WL WD WW] = {g["penta"]}')
    print(f'   score {m:.4f}; Elo (pentanomial) {e:+.1f} [{e - ci:+.1f}, {e + ci:+.1f}] 95% CI')
    print(f'   losses by colour: black {g["loss_col"]["black"]}, white {g["loss_col"]["white"]}')
    if g['losses']:
        print('   losses by ballot: ' + '; '.join(f'{o} ({c}) x{n}' if n > 1 else f'{o} ({c})'
                                               for (o, c), n in sorted(g['losses'].items())))
    n = g['hits'] + g['misses']
    if n:
        n50 = g['hits50'] + g['misses50']
        print(f'   ponder: {g["hits"]} hits, {g["misses"]} misses, hit rate {100 * g["hits"] / n:.1f}%; '
              f'where the opponent took >= 50 ms: {g["hits50"]}/{n50} = {100 * g["hits50"] / max(1, n50):.1f}%')
    for lab, recs in (('searched', g['allm']), ('  hits', g['hitm']), ('  no hit', g['nohit']),
                      ('first25', g['early']), ('  hits', g['ehit']), ('  no hit', g['enohit'])):
        if recs:
            print(f'   {lab:9s} moves {len(recs):6d}: median depth {med(recs, 0):.0f}, kN/s {med(recs, 1):.0f}, '
                  f'nodes {med(recs, 2):.0f}, time {med(recs, 3):.2f}s; mean depth {S.mean(r[0] for r in recs):.2f}')
if len(groups) > 1:
    a = groups[0]
    for b in groups[1:]:
        d = b[3] - a[3]
        sd = math.sqrt((a[2] * a[4]) ** 2 + (b[2] * b[4]) ** 2)
        z = d / sd if sd else 0
        print(f'{b[0]} - {a[0]}: {d:+.1f} Elo [{d - 1.96 * sd:+.1f}, {d + 1.96 * sd:+.1f}] 95% CI, '
              f'LOS {50 * (1 + math.erf(z / math.sqrt(2))):.1f}%')
