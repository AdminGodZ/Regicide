# Timing markers for the Cake losses (see cake_phase.py for parsing): (a) Cake's eval first >= +50 and never below +25
# afterwards; (b) Regicide's own score first <= -100 and never above -50 afterwards. Phase of each marker by Regicide
# move number after the ballot and piece count: opening = within 10 moves after the ballot, middlegame = later with
# > 12 pieces, endgame = <= 12 pieces.
import re, os, csv, subprocess, collections, sys
sys.argv = [sys.argv[0]]
exec(open('/mnt/c/regicide-match/jobs/cake/cake_phase.py').read().split('# piece counts at the decisive ply')[0])
def first_sustained(vals, hit, keep):
    ks = sorted(vals)
    for j, i in enumerate(ks):
        if hit(vals[i]) and all(keep(vals[k]) for k in ks[j:]): return i
    return None
for x in rows:
    x['a'] = first_sustained(x['cake'], lambda v: v >= 50, lambda v: v >= 25)
    x['b'] = first_sustained(x['reg'], lambda v: v <= -100, lambda v: v <= -50)
cmds, idx = [], []
for x in rows:
    for k in ('a', 'b'):
        if x[k] is not None:
            cmds.append('position startpos moves ' + ' '.join(x['moves'][:x[k]]) + '\nd\n'); idx.append((x, k))
out = subprocess.run([E], input=''.join(cmds) + 'quit\n', capture_output=True, text=True).stdout
for (x, k), f in zip(idx, re.findall(r'^([BW]:[^\s]+)$', out, re.M)):
    x[k + 'p'] = len(re.findall(r'K?\d+', f))
def phase(ply, pcs): return 'opening' if ply < 26 else ('endgame' if pcs <= 12 else 'middlegame')
for k, lab in (('a', "Cake eval >= +50 sustained"), ('b', "Regicide score <= -100 sustained")):
    c = collections.Counter(phase(x[k], x[k + 'p']) if x[k] is not None else 'never' for x in rows)
    mv = sorted((x[k] - 6) // 2 + 1 for x in rows if x[k] is not None)
    print(f'{lab:34s}: {dict(c)}; move after ballot: median {mv[len(mv)//2]}, quartiles {mv[len(mv)//4]}-{mv[3*len(mv)//4]}')
print(f'{"run":14s} {"game":>5s} {"col":3s} {"ballot":26s} {"plies":>5s} | {"(a) mv":>6s} {"pcs":>4s} {"phase":10s} | {"(b) mv":>6s} {"pcs":>4s} {"phase":10s}')
for x in sorted(rows, key=lambda x: (x['opening'], x['run'])):
    f = lambda k: (f'{(x[k]-6)//2+1:6d} {x[k+"p"]:4d} {phase(x[k], x[k+"p"]):10s}' if x[k] is not None else f'{"-":>6s} {"":4s} {"never":10s}')
    print(f'{x["run"]:14s} {x["game"]:5d} {x["colour"]:3s} {x["opening"][:26]:26s} {x["plies"]:5d} | {f("a")} | {f("b")}')
