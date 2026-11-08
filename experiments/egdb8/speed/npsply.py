# Regicide searched-move nps by Regicide move number (same game phase), 7-piece control vs 8-piece, KingsRow runs;
# plus the eb1037c-n4c 7-piece runs for Cake/GuiNN vs the 8-piece ones (load differs there).
import re, glob, statistics
pat = re.compile(r'\{(\d+\.\d+)s depth (\d+)/\d+ score (-?\d+) nodes (\d+) (\d+) kN/s')
def by_move(name):
    b = {}
    for f in glob.glob(f'/home/admingod/regicide-match/results/baseline-{name}/games/*.pdn'):
        for i, m in enumerate(pat.finditer(open(f).read().replace('\n', ' '))):
            if float(m.group(1)) >= 0.3: b.setdefault(min(i // 5, 8), []).append(int(m.group(5)))
    return b
rows = [('KingsRow 7-piece control (gen5 load)', '30f09c3-kingsrow'), ('KingsRow 8-piece (gen5 load)', '30f09c3-egdb8-kingsrow'),
        ('KingsRow 7-piece eb1037c (gen4 load)', 'eb1037c-n4c-kingsrow'),
        ('Cake 7-piece eb1037c (gen4 load)', 'eb1037c-n4c-cake'), ('Cake 8-piece (gen5 load)', '30f09c3-egdb8-cake')]
print('median kN/s of searched Regicide moves (>= 0.3 s) by Regicide move index: 0-4, 5-9, ..., 35-39, 40+   [count]')
for lab, n in rows:
    b = by_move(n)
    print(f'{lab:38s}', ' '.join(f'{statistics.median(b[k]):5.0f}[{len(b[k])}]' if k in b else '   -   ' for k in range(9)))
