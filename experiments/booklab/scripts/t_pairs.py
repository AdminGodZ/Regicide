import os, sys, glob, shutil
sys.path.insert(0, '/mnt/c/regicide-match/jobs/booklab/src/tools/booklab')
import wp12_discover as D, wp12_gates as G
L = os.path.expanduser('~/regicide-jobs/booklab'); R = os.path.expanduser('~/regicide-match/results')
T = L + '/t_pairs'; shutil.rmtree(T, ignore_errors=True); os.makedirs(T)
src = R + '/bl-e2d-cake163-natural/games'
g1, g2 = sorted(glob.glob(src + '/*.pdn'))[:2]
open(T + '/games.pdn', 'w').write(open(g1).read().rstrip() + '\n\n' + open(g2).read())
os.makedirs(L + '/wp12/books', exist_ok=True)
book = L + '/wp12/books/selftest.bin'
if not os.path.exists(book): os.symlink(L + '/book_r11d.bin', book)
gs = G.load_games(T + '/games.pdn'); gw = next(g for g in gs if g['col'] == 'W')
rows = G.pinwalk(D.PINWALK, book, D.EGDB, [gw])[0]
r = next(r for r in rows if r['eligible'] and r['book'] != '-' and r['ply'] > 8)
print('key', r['ply'] + 1, r['fen'], r['book'])
D.PAIRS = True
c = dict(name='selftest', colour='W', fen=r['fen'], alt=r['book'])
print('W candidate:', D.analyse(c, T))
print('B candidate (key of the W game, so never reached):', D.analyse(dict(c, colour='B'), T))
D.PAIRS = False
try: D.analyse(c, T)
except ValueError as e: print('single mode refuses a pair:', e)
os.remove(book); shutil.rmtree(T)
