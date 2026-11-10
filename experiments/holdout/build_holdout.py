#!/usr/bin/env python3
"""Build deterministic, leakage-audited English-checkers opening panels.

Copy this file and holdout_helper.cpp to experiments/holdout/. Requires Python
3.9+, Git, CMake, Ninja and the Clang/C++26 toolchain required by Regicide.
No engine search, match, network access or package installation is performed.
All source archives and build products live under OUT/.work, not in the checkout.
"""
import argparse
import csv
import hashlib
import io
import json
import math
import os
from pathlib import Path
import re
import shutil
import statistics
import subprocess
import tarfile
import time

MASTER = "456e9b224a380771c50a1b0da9360757c16aa62e"
BOOK = "3978ecd94f6f0f29a255b90d06a09eab9fb21c42"
OLD = "fcf6acbce2bf3c0e211c6da27eea59b446cf6b51"
SEED = 20261110
BATCH = 25
BOUND = 12


def run(cmd, cwd=None, timeout=120, log=None):
    result = subprocess.run([str(x) for x in cmd], cwd=cwd,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            timeout=timeout)
    if log:
        Path(log).write_bytes(result.stdout)
    if result.returncode:
        raise RuntimeError("command failed: %s\n%s" %
                           (" ".join(map(str, cmd)), result.stdout.decode(errors="replace")))
    return result.stdout


def git(repo, *args):
    return run(["git", "-C", repo, *args])


def digest(data):
    return hashlib.sha256(data).hexdigest()


def tsv_read(path):
    with Path(path).open(newline="") as f:
        return list(csv.DictReader(f, delimiter="\t"))


def tsv_write(path, rows):
    with Path(path).open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0]), delimiter="\t",
                                lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def queries(path, rows):
    Path(path).write_text("".join(str(i) + "\t" + moves + "\n" for i, moves in rows))


def extract_source(repo, sha, dest):
    archive = git(repo, "archive", sha)
    dest.mkdir(parents=True, exist_ok=True)
    with tarfile.open(fileobj=io.BytesIO(archive)) as tar:
        # Git-controlled source, with an additional portable path/type check.
        for member in tar.getmembers():
            path = Path(member.name)
            if path.is_absolute() or ".." in path.parts:
                raise RuntimeError("unsafe archive path: " + member.name)
            if not member.isfile() and not member.isdir():
                raise RuntimeError("non-regular archive member: " + member.name)
        if hasattr(tarfile, "data_filter"):
            tar.extractall(dest, filter="data")
        else:
            tar.extractall(dest)


def quantile(values, p):
    x = sorted(values)
    t = (len(x) - 1) * p
    i = int(math.floor(t))
    return x[i] + (x[min(i + 1, len(x) - 1)] - x[i]) * (t - i)


def evaluation_summary(rows):
    x = [int(r["eval_black"]) for r in rows]
    if not x:
        return {"n": 0}
    return {"n": len(x), "point_of_view": "Black", "unit": "centi-men",
            "min": min(x), "q25": quantile(x, .25),
            "median": quantile(x, .5), "q75": quantile(x, .75),
            "max": max(x), "mean": round(statistics.mean(x), 6),
            "favours_black": sum(v > 0 for v in x),
            "favours_white": sum(v < 0 for v in x), "zero": x.count(0)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, default=Path.cwd())
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--master-ref", default=MASTER)
    parser.add_argument("--book-ref", default=BOOK)
    parser.add_argument("--old-ref", default=OLD)
    parser.add_argument("--seed", type=int, default=SEED)
    parser.add_argument("--size", type=int, default=156)
    parser.add_argument("--cap", type=int, default=50000,
                        help="maximum expanded uncached states per colour/root")
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--cxx", default=os.environ.get("CXX", "clang++"))
    args = parser.parse_args()
    if not 1 <= args.jobs <= 4 or args.cap < 1 or args.size < 1:
        parser.error("jobs must be 1..4, cap and size positive")
    repo = args.repo.resolve()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    work = out / ".work"
    work.mkdir(exist_ok=True)
    sources = {label: git(repo, "rev-parse", ref + "^{commit}").decode().strip()
               for label, ref in [("master", args.master_ref),
                                  ("booklab", args.book_ref), ("old_master", args.old_ref)]}
    source = work / ("source-" + sources["master"][:12])
    if not (source / "CMakeLists.txt").exists():
        extract_source(repo, sources["master"], source)
    source_files = {
        "decisions": (sources["booklab"], "experiments/booklab/decisions.txt"),
        "candidate_book": (sources["booklab"], "experiments/booklab/book_booklab.bin"),
        "openings4": (sources["master"], "experiments/eval-gen1/openings4.txt"),
        "deck_table": (sources["master"], "tools/match/openings.cpp"),
        "builtin_net": (sources["master"], "src/weights/net.bin"),
        "old_book": (sources["old_master"], "src/weights/book.bin"),
        "old_net": (sources["old_master"], "src/weights/net.bin"),
        "master_book": (sources["master"], "src/weights/book.bin"),
    }
    data = {name: git(repo, "show", sha + ":" + path)
            for name, (sha, path) in source_files.items()}
    inputs = work / "inputs"
    inputs.mkdir(exist_ok=True)
    decisions = inputs / "decisions.txt"
    book = inputs / "candidate.bin"
    decisions.write_bytes(data["decisions"])
    book.write_bytes(data["candidate_book"])
    cxx = shutil.which(args.cxx)
    if not cxx:
        raise RuntimeError("C++ compiler not found: " + args.cxx)
    build = work / "build"
    run(["cmake", "-S", source, "-B", build, "-G", "Ninja",
         "-DCMAKE_CXX_COMPILER=" + cxx, "-DREGICIDE_LTO=OFF"],
        log=work / "configure.log")
    run(["cmake", "--build", build, "--target", "regicide_core", "cbt_common",
         "-j" + str(args.jobs)], log=work / "build.log")
    cpp = Path(__file__).resolve().with_name("holdout_helper.cpp")
    if not cpp.is_file():
        raise RuntimeError("holdout_helper.cpp must be beside this script")
    helper = work / "holdout_helper"
    run([cxx, "-std=c++26", "-O3", "-DNDEBUG", "-march=native",
         "-I" + str(source / "src"), "-I" + str(source / "tools/common"),
         "-I" + str(source / "tools/match"), cpp,
         source / "tools/match/openings.cpp", build / "libregicide_core.a",
         build / "libregicide_egdb.a", build / "tools/libcbt_common.a",
         "-pthread", "-o", helper], log=work / "helper-build.log")

    def classify(rows, policy, do_c, name, action_budget=BOUND + 1, colour=None):
        combined = []
        elapsed = []
        for start in range(0, len(rows), BATCH):
            inp = work / (name + "-input.tsv")
            target = work / (name + "-output.tsv")
            queries(inp, rows[start:start + BATCH])
            begin = time.monotonic()
            run([helper, "--input", inp, "--decisions", decisions, "--book", book,
                 "--out", target, "--plies", str(action_budget), "--cap", str(args.cap),
                 "--miss-policy", policy, "--do-c", str(int(do_c))] +
                (["--colour", colour] if colour else []), timeout=12)
            elapsed.append(time.monotonic() - begin)
            result = tsv_read(target)
            if len(result) != len(rows[start:start + BATCH]):
                raise RuntimeError("helper omitted an opening")
            combined.extend(result)
            if do_c:
                print(name, min(start + BATCH, len(rows)), "/", len(rows), flush=True)
        tsv_write(out / (name + ".tsv"), combined)
        runtime[name] = {"total_seconds": sum(elapsed),
                         "maximum_batch_seconds": max(elapsed, default=0)}
        return {r["id"]: r for r in combined}

    def strict_positions(rows, name):
        # First resolve decision actions 1..12. A positive certificate is already
        # valid for the inclusive position horizon and needs no deeper search.
        initial = classify(rows, "legal", True, name + "_initial", action_budget=BOUND)
        result = {i: dict(row, initial_nodes_black=row["nodes_black"],
                          initial_nodes_white=row["nodes_white"], boundary_nodes="0")
                  for i, row in initial.items()}
        moves = dict(rows)
        for colour, field in [("B", "black"), ("W", "white")]:
            # Depth 12 is even. Only the opening's side to move can own a new
            # target at that boundary; the other colour's goals end at depth 11.
            pending = [(i, moves[i]) for i, row in initial.items()
                       if row["c_black"] == row["c_white"] == "clean"
                       and row["final_fen"].startswith(colour + ":")]
            if not pending:
                continue
            boundary = classify(pending, "legal", True, name + "_boundary_" + colour,
                                action_budget=BOUND + 1, colour=colour)
            for i, row in boundary.items():
                target = result[i]
                target["c_" + field] = row["c_" + field]
                target["witness_" + field] = row["witness_" + field]
                target["boundary_nodes"] = row["nodes_" + field]
                target["nodes_" + field] = str(int(target["nodes_" + field]) +
                                               int(row["nodes_" + field]))
        tsv_write(out / (name + ".tsv"), list(result.values()))
        return result

    runtime = {}
    deck_re = r'\{(\d+), "([^\"]+)", \'(.)\', (?:"([^\"]+)"|nullptr)\}'
    deck = [{"acf": int(i), "moves": moves, "kind": kind, "name": name}
            for i, moves, kind, name in re.findall(deck_re, data["deck_table"].decode())]
    if len(deck) != 174 or len({r["acf"] for r in deck}) != 174:
        raise RuntimeError("could not identify the 174-row ACF deck")
    ballot_rows = classify([("B%03d" % r["acf"], r["moves"]) for r in deck],
                           "stop", False, "ballot_inventory")
    decision_rows = [r.split("|", 3) for r in data["decisions"].decode().splitlines()
                     if r.strip() and not r.lstrip().startswith("#")]
    affected = {int(i) for row in decision_rows
                for i in re.findall(r"ACF #(\d+)", row[3])}
    if any(not re.search(r"ACF #(\d+)", r[3]) for r in decision_rows):
        raise RuntimeError("a decision lacks ballot provenance")
    min_pieces = min(sum(len(part[1:].split(",")) for part in r[0].strip().split(":")[1:])
                     for r in decision_rows if r[2].strip() == "force")
    if min_pieces <= 8:
        raise RuntimeError("DB-covered decision: extend DB eligibility handling before using this generator")
    affected_positions = {ballot_rows["B%03d" % r["acf"]]["final_fen"]
                          for r in deck if r["acf"] in affected}
    raw = [r.strip() for r in data["openings4"].decode().splitlines()
           if r.strip() and not r.lstrip().startswith("#")]
    if len(raw) != 805 or any(len(r.split()) != 4 for r in raw):
        raise RuntimeError("expected 805 four-ply opening sequences")
    source_moves = {str(i + 1): moves for i, moves in enumerate(raw)}
    roots = [(i, moves) for i, moves in source_moves.items()]
    first3 = classify([(i, " ".join(moves.split()[:3])) for i, moves in roots],
                      "stop", False, "first_three_inventory")
    narrow = classify(roots, "stop", True, "classification_narrow")
    strict = strict_positions(roots, "classification_strict")
    if len({r["final_fen"] for r in strict.values()}) != 805:
        raise RuntimeError("source openings contain final-board transpositions")
    acf_positions = {r["final_fen"] for r in ballot_rows.values()}
    unlisted_first_three = {i for i in source_moves
                            if first3[i]["final_fen"] not in acf_positions}
    # openings4 enumerates legal four-ply positions, not only the 174 ballot texts.
    # An unlisted legal prefix is not an affected ACF ballot under rule (a).
    rule_a = {i for i in source_moves if first3[i]["final_fen"] in affected_positions}
    rule_a_exact = {i for i, m in source_moves.items()
                    if " ".join(m.split()[:3]) in {r["moves"] for r in deck if r["acf"] in affected}}
    rule_b = {i for i, r in narrow.items() if r["rule_b"] == "1"}
    contact = lambda rows: {i for i, r in rows.items()
                            if "touched" in (r["c_black"], r["c_white"])}
    narrow_c = contact(narrow)
    strict_c = contact(strict) | narrow_c  # A narrow witness is also a valid strict witness.
    tier1 = rule_a | rule_b | narrow_c
    strict_only = strict_c - tier1
    clean = {i for i, r in strict.items() if r["c_black"] == r["c_white"] == "clean"} - tier1
    unresolved = set(source_moves) - tier1 - strict_only - clean
    if len(tier1) > args.size or len(strict_only) < args.size - len(tier1):
        raise RuntimeError("cannot fill touched panel: tier1=%d strict-only=%d size=%d; preserve the rule and increase cap if appropriate" %
                           (len(tier1), len(strict_only), args.size))

    def rank(i):
        return digest((str(args.seed) + "\0" + strict[i]["final_fen"]).encode())

    h2 = sorted(clean, key=rank)[:args.size]
    h2x1 = sorted(tier1, key=rank)
    h2x2 = sorted(strict_only, key=rank)[:args.size - len(h2x1)]
    h2x = h2x1 + h2x2
    h1 = [r for r in deck if r["kind"] == "B"]
    if len(h1) != 18:
        raise RuntimeError("expected 18 barred ballots")
    h1_ids = ["B%03d" % r["acf"] for r in h1]
    h1_narrow = classify([(i, r["moves"]) for i, r in zip(h1_ids, h1)],
                         "stop", True, "h1_narrow_contact")
    h1_strict = strict_positions([(i, r["moves"]) for i, r in zip(h1_ids, h1)],
                                 "h1_strict_contact")

    # Replay every positive certificate through the referee and check book policy,
    # including the final forced move. This validates positive contact independently
    # of the DFS ordering, cache and lower-bound pruning.
    witness_queries = []
    for label, rows in [("narrow", narrow), ("strict", strict)]:
        for i, row in rows.items():
            for colour, name in [("B", "black"), ("W", "white")]:
                if row["c_" + name] == "touched":
                    witness_queries.append((label + "/" + i + "/" + colour,
                                            source_moves[i], colour, row["witness_" + name]))
    winput = work / "witnesses.tsv"
    winput.write_text("".join("\t".join(q) + "\n" for q in witness_queries))
    woutput = out / "witness_validation.tsv"
    run([helper, "--witness-input", winput, "--decisions", decisions,
         "--book", book, "--out", woutput], timeout=12)
    witness_info = {r["id"]: r for r in tsv_read(woutput)}

    def record(i, tier=None):
        row = strict[i]
        r = {"source_line": int(i), "moves": source_moves[i], "final_fen": row["final_fen"],
             "eval_stm": int(row["eval_stm"]), "eval_black": int(row["eval_black"]),
             "rule_a": i in rule_a, "rule_b": i in rule_b,
             "rule_c_narrow": i in narrow_c, "rule_c_strict_proved": i in strict_c,
             "strict_black": row["c_black"], "strict_white": row["c_white"],
             "rank_sha256": rank(i)}
        if tier is not None:
            r["tier"] = tier
        if tier == 2:
            colour = "B" if row["c_black"] == "touched" else "W"
            cert = witness_info["strict/" + i + "/" + colour]
            if int(cert["first_own_book_miss"]) <= 0:
                raise RuntimeError("tier 2 witness did not exit the continuous book line")
            r["witness"] = {"colour": colour, "moves": row["witness_" + ("black" if colour == "B" else "white")],
                            "first_own_book_miss_ply": int(cert["first_own_book_miss"]),
                            "decision_fen": cert["decision_fen"], "forced_move": cert["forced_move"]}
        return r

    h1_records = [{"acf": r["acf"], "name": r["name"], "kind": "B", "moves": r["moves"],
                   "final_fen": ballot_rows[i]["final_fen"],
                   "eval_stm": int(ballot_rows[i]["eval_stm"]),
                   "eval_black": int(ballot_rows[i]["eval_black"]),
                   "reason_recorded_in_repository": None}
                  for i, r in zip(h1_ids, h1)]
    panels = {"h1_barred.txt": h1_records,
              "h2_holdout156.txt": [record(i) for i in h2],
              "h2x_touched156.txt": [record(i, 1 if i in tier1 else 2) for i in h2x]}
    for filename, records in panels.items():
        lines = ["# Regicide holdout panel: " + filename,
                 "# Actual openings: %d; each is played with both colours." % len(records),
                 "# master=" + sources["master"], "# booklab=" + sources["booklab"],
                 "# seed=%d; no static-evaluation filtering." % args.seed]
        for r in records:
            label = ("ACF %d kind B" % r["acf"]) if "acf" in r else (
                "source openings4 line %d" % r["source_line"] +
                ("; tier %d" % r["tier"] if "tier" in r else ""))
            lines.extend(["# " + label, r["moves"]])
            r["file_line"] = len(lines)
        (out / filename).write_text("\n".join(lines) + "\n")
        rvalidation = run([helper, "--validate", out / filename], timeout=12).decode().strip()
        if not rvalidation.startswith(str(len(records)) + " openings passed"):
            raise RuntimeError("panel validation count mismatch")
        print(filename, rvalidation, flush=True)
    if set(h2) & set(h2x):
        raise RuntimeError("clean and touched panels overlap")
    h1_production_overlap = {r["final_fen"] for r in h1_records} & {
        ballot_rows["B%03d" % r["acf"]]["final_fen"] for r in deck if r["kind"] != "B"}
    if h1_production_overlap:
        raise RuntimeError("a barred final position is in acf156")

    audit = []
    for i in source_moves:
        row = strict[i]
        audit.append({"source_line": i, "moves": source_moves[i], "first_three_fen": first3[i]["final_fen"],
                      "final_fen": row["final_fen"], "eval_black": row["eval_black"],
                      "rule_a": int(i in rule_a), "rule_b": int(i in rule_b),
                      "c_narrow": int(i in narrow_c), "c_strict_proved": int(i in strict_c),
                      "strict_black": row["c_black"], "strict_white": row["c_white"],
                      "nodes_black": row["nodes_black"], "nodes_white": row["nodes_white"],
                      "overall": "tier1" if i in tier1 else "strict_only" if i in strict_only else
                      "clean" if i in clean else "unresolved", "rank_sha256": rank(i)})
    tsv_write(out / "opening_audit.tsv", audit)
    rule_counts = {
        "source_openings": 805, "source_unique_final_positions": 805,
        "first_three_not_in_acf174": len(unlisted_first_three),
        "a_first_three_canonical_ballot": len(rule_a), "a_exact_text_only": len(rule_a_exact),
        "b_any_opening_path_position": len(rule_b), "c_narrow_proved": len(narrow_c),
        "narrow_union_tier1": len(tier1), "c_strict_proved": len(strict_c),
        "c_strict_exhausted_clean": sum(r["c_black"] == r["c_white"] == "clean" for r in strict.values()),
        "c_strict_unresolved_without_contact_proof": 805 - len(strict_c) -
            sum(r["c_black"] == r["c_white"] == "clean" for r in strict.values()),
        "strict_only_resolved_pool": len(strict_only), "strict_overall_clean_pool": len(clean),
        "strict_overall_unresolved_excluded": len(unresolved),
        "exclusive_a_then_b_then_narrow_c": [len(rule_a), len(rule_b - rule_a),
                                              len(narrow_c - rule_a - rule_b)],
        "overlaps": {"a_and_b": len(rule_a & rule_b), "a_and_narrow_c": len(rule_a & narrow_c),
                     "b_and_narrow_c": len(rule_b & narrow_c)},
    }
    manifest = {
        "schema": 1, "seed": args.seed, "requested_panel_size": args.size,
        "source_commits": sources,
        "sources": {name: {"commit": sha, "path": path, "bytes": len(data[name]),
                            "sha256": digest(data[name])}
                    for name, (sha, path) in source_files.items()},
        "decision_rows": len(decision_rows), "affected_acf_ballots": sorted(affected),
        "minimum_forced_decision_pieces": min_pieces,
        "definition": {"plies_after_opening": BOUND,
                       "decision_position_depths": "0..12 inclusive",
                       "certificate_may_include_forced_action_13": True,
                       "colours": ["Black", "White"], "cap_per_pass_colour_uncached_states": args.cap,
                       "strict_initial_action_budget": BOUND,
                       "strict_boundary_action_budget": BOUND + 1,
                       "strict_boundary_policy": "recheck only initially exhausted clean cases; opening side to move only",
                       "batch_size": BATCH, "narrow_own_miss": "end line",
                       "strict_own_miss": "all legal non-avoided moves; all moves if all avoided",
                       "opponent": "all legal replies", "book_margin": 0,
                       "king_run_guard": "book is inactive when reversible_plies > 0",
                       "draw_history": "ignored in enumeration (conservative); replayed witnesses may not continue past a draw",
                       "database": "no probe required: all decision targets have >8 pieces and material cannot increase",
                       "cap_policy": "no proof plus a cap is unresolved and excluded; an existence proof resolves contact",
                       "selection": "ascending SHA256(decimal seed + NUL + canonical final FEN)",
                       "selection_uses_evaluation": False,
                       "evaluation": "master builtin net, static only; Black POV for common summaries",
                       "quantiles": "linear interpolation at (n-1)*p"},
        "counts": rule_counts,
        "h1_diagnostics": {"acf_ids": [r["acf"] for r in h1],
                           "acf156_final_position_overlap": 0,
                           "narrow_c_proved": len(contact(h1_narrow)),
                           "strict_c_proved": len(contact(h1_strict)),
                           "strict_c_exhausted_clean": sum(r["c_black"] == r["c_white"] == "clean"
                                                           for r in h1_strict.values()),
                           "reason_in_table": "B means barred; no per-ballot explanation or lost side is supplied"},
        "panels": {},
        "builder": {"python_sha256": digest(Path(__file__).read_bytes()),
                    "cpp_sha256": digest(cpp.read_bytes())},
    }
    for filename, records in panels.items():
        manifest["panels"][filename] = {"count": len(records), "games_per_repetition": 2 * len(records),
                                         "sha256": digest((out / filename).read_bytes()),
                                         "static_eval_black": evaluation_summary(records), "openings": records}
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    checksums = [filename for filename in panels] + ["manifest.json", "opening_audit.tsv", "witness_validation.tsv"]
    (out / "SHA256SUMS").write_text("".join(digest((out / f).read_bytes()) + "  " + f + "\n" for f in checksums))
    (work / "build_info.json").write_text(json.dumps({"compiler": run([cxx, "--version"]).decode(),
                                                      "runtime": runtime}, indent=2))
    print(json.dumps({"counts": rule_counts,
                      "panels": {f: {k: v for k, v in x.items() if k != "openings"}
                                 for f, x in manifest["panels"].items()}}, indent=2))


if __name__ == "__main__":
    main()
