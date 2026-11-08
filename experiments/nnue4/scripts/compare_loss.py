# Score evals on the NNUE trainer's own validation set (seed, valid_frac, valid_max from CONFIG) with its K and lambda.
# usage: compare_loss.py CONFIG.json OUTDIR ENGINE name=evalfile ...
import json, os, subprocess, sys
import numpy as np
sys.path.insert(0, os.path.expanduser('~/regicide-385/tools/nnue'))
import nnue
cfg = json.load(open(sys.argv[1])); out = sys.argv[2]; engine = sys.argv[3]
K, lam = cfg['K'], cfg['lam']
data = nnue.Data(cfg['data'], cfg['valid_frac'])
valid = data.validation(cfg['valid_max'], np.random.default_rng(cfg['seed']))
vf = os.path.join(out, 'nnue_valid_1M.bin')
valid.tofile(vf)
y = lam * nnue.stm_result(valid) + (1 - lam) / (1 + np.exp(-valid['score'].astype(np.float64) / K))
print('valid positions %d  K %.2f  lambda %.2f  engine %s' % (len(valid), K, lam, engine))
for spec in sys.argv[4:]:
    name, ev = spec.split('=', 1)
    r = subprocess.run([engine, 'evaldata', ev, vf], capture_output=True, text=True, check=True)
    e = np.array([float(l.split()[-1]) for l in r.stdout.split('\n') if l.strip()])
    assert len(e) == len(valid), (len(e), len(valid))
    z = e / K
    loss = np.mean(np.logaddexp(0, z) - y * z)   # BCE with logits
    print('%-28s %.6f' % (name, loss))
