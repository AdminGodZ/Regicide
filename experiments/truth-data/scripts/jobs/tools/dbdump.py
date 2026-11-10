import re
p = "/home/admingod/regicide-truth/experiments/truth-data/scripts/db_truth.py"
t = open(p).read()
t = t.replace('    p.add_argument("--general-n", type=int, default=200000)\n',
 '    p.add_argument("--general-n", type=int, default=200000)\n    p.add_argument("--dump", help="write the per-record evals (gzip CSV: set, index in set, pieces, proven sign, eval per net)")\n')
t = t.replace('    rec = np.concatenate([np.fromfile(f, nnue.RECORD) for f in a.set])\n    rec = rec[nnue.proven_of(rec)]\n',
 '    parts = [np.fromfile(f, nnue.RECORD) for f in a.set]\n    src = np.concatenate([np.full(len(x), i) for i, x in enumerate(parts)])\n    idx = np.concatenate([np.arange(len(x)) for x in parts])\n    rec = np.concatenate(parts)\n    keep = nnue.proven_of(rec)\n    rec, src, idx = rec[keep], src[keep], idx[keep]\n    dump = {}\n')
t = t.replace('        agree = np.sign(ev) == sign\n', '        dump[name] = ev\n        agree = np.sign(ev) == sign\n')
NL = chr(92) + "n"
block = ('\n    if a.dump:\n        import gzip\n        with gzip.open(a.dump, "wt") as f:\n'
         '            f.write("set,index,pieces,proven_sign," + ",".join(dump) + "' + NL + '")\n'
         '            for i in range(len(rec)):\n'
         '                f.write("%s,%d,%d,%d,%s' + NL + '" % (os.path.basename(a.set[src[i]]), idx[i], pieces[i], sign[i],\n'
         '                                              ",".join(str(int(dump[n][i])) for n in dump)))\n')
t = t.replace('\n\nif __name__ == "__main__":', block + '\n\nif __name__ == "__main__":')
open(p, "w").write(t); print("ok")
