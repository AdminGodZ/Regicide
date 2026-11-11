#!/usr/bin/env python3
"""Tests of the WP12 gates on made-up records (reviewer's reference cases, 2026-11-11), the consulted rule and the
on-board rule. Run: python3 tests_wp12_gates.py"""
import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import wp12_gates as G  # noqa: E402


def s2(cand_w=4, cand_l=0, exposed_w=None, ctrl_w=1, blocks=3, off=False):
    """12 candidate and 12 control games spread over `blocks` blocks; cand_w wins of which exposed_w consulted the key
    (default all), cand_l losses, the rest drawn; ctrl_w control wins."""
    exposed_w = cand_w if exposed_w is None else exposed_w
    recs = []
    for i in range(12):
        res = "W" if i < cand_w else ("L" if i < cand_w + cand_l else "D")
        recs.append(dict(arm="cand", block=f"b{i % blocks}", result=res, consulted=int(i < exposed_w or res != "W"),
                         off_board=False, term="threefold repetition"))
        recs.append(dict(arm="ctrl", block=f"b{i % blocks}", result="W" if i < ctrl_w else "D", consulted=1,
                         off_board=False, term="threefold repetition"))
    if off:
        recs[0]["off_board"], recs[0]["term"] = True, "Cake lost on time (0.9s used, 0.5s left)"
    return recs


class Stage2(unittest.TestCase):
    def test_keep_4_8_0_vs_1_in_3_blocks(self):
        v, why, c = G.eval_stage2(s2())
        self.assertEqual(v, "keep", why)
        self.assertEqual((c["cand_W"], c["cand_D"], c["cand_L"], c["exposed_wins"], c["ctrl_W"], c["blocks"]),
                         (4, 8, 0, 4, 1, 3))

    def test_reject_3_exposed_wins(self):
        self.assertEqual(G.eval_stage2(s2(cand_w=3))[0], "reject")

    def test_reject_4_wins_only_3_exposed(self):
        v, why, _ = G.eval_stage2(s2(cand_w=4, exposed_w=3))
        self.assertEqual(v, "reject")
        self.assertTrue(any("consulted" in w for w in why))

    def test_reject_one_loss(self):
        self.assertEqual(G.eval_stage2(s2(cand_w=4, cand_l=1))[0], "reject")

    def test_reject_2_control_wins(self):
        self.assertEqual(G.eval_stage2(s2(ctrl_w=2))[0], "reject")

    def test_reject_2_blocks(self):
        self.assertEqual(G.eval_stage2(s2(blocks=2))[0], "reject")

    def test_off_board_not_counted(self):
        self.assertEqual(G.eval_stage2(s2(off=True))[0], "not counted")

    def test_incomplete_not_counted(self):
        self.assertEqual(G.eval_stage2(s2()[:-2])[0], "not counted")


class Stage3(unittest.TestCase):
    PLAN = G.stage3_plan(["163W", "165W"], "163W")

    def recs(self, consulted=12, loss=None):
        out = []
        for o, ctx in self.PLAN:
            for i in range(12):
                res = "L" if loss and loss[:2] == (o, ctx) and i == 0 else "D"
                cons = int(i < consulted) if not (loss and loss[:2] == (o, ctx) and i == 0) else loss[2]
                out.append(dict(opp=o, context=ctx, result=res, consulted=cons, off_board=False, term="threefold repetition"))
        return out

    def test_plan(self):
        self.assertEqual(self.PLAN, [("kingsrow", "163W"), ("guinn", "163W"), ("cake", "165W"), ("kingsrow", "165W"),
                                     ("guinn", "165W")])

    def test_pass(self):
        self.assertEqual(G.eval_stage3(self.recs(), self.PLAN)[0], "pass")

    def test_exposed_loss_rejects(self):
        self.assertEqual(G.eval_stage3(self.recs(loss=("guinn", "165W", 1)), self.PLAN)[0], "reject")

    def test_unexposed_loss_reported_not_rejecting(self):
        v, why, _ = G.eval_stage3(self.recs(loss=("guinn", "165W", 0)), self.PLAN)
        self.assertEqual(v, "pass")
        self.assertTrue(any("did not consult" in w for w in why))

    def test_untested(self):
        # 2 consultations per pair: kingsrow 4 (< 6) untested; guinn 4 untested; cake 2
        v, why, _ = G.eval_stage3(self.recs(consulted=2), self.PLAN)
        self.assertEqual(v, "pass")
        self.assertEqual(sum("untested" in w for w in why), 3)

    def test_incomplete(self):
        self.assertEqual(G.eval_stage3(self.recs()[:-1], self.PLAN)[0], "not counted")


class Rules(unittest.TestCase):
    ROW = dict(ply=4, fen="F", move="8-12", eligible=True, book="8-12", avoided=set())
    BOOK = "0.00s depth 0/0 score 0 nodes 0 0 kN/s pv 8-12"
    SEARCH = "0.48s depth 21/30 score 4 nodes 812345 1600 kN/s pv 8-12 24-20"

    def test_consulted_book_move(self):
        self.assertEqual(G.key_use([self.ROW], [None] * 4 + [self.BOOK], "F", "8-12")[:2], (1, 1))

    def test_searched_equal_move_is_not_consulted(self):
        self.assertEqual(G.key_use([self.ROW], [None] * 4 + [self.SEARCH], "F", "8-12")[:2], (1, 0))

    def test_book_plays_other_move(self):
        row = dict(self.ROW, book="11-15")
        self.assertEqual(G.key_use([row], [None] * 4 + [self.BOOK], "F", "8-12")[:2], (1, 0))

    def test_not_eligible(self):
        row = dict(self.ROW, eligible=False)
        self.assertEqual(G.key_use([row], [None] * 4 + [self.BOOK], "F", "8-12")[:2], (1, 0))

    def test_not_reached(self):
        self.assertEqual(G.key_use([dict(self.ROW, fen="X")], [None] * 4 + [self.BOOK], "F", "8-12")[:2], (0, 0))

    def test_on_board(self):
        for t in ("white has no legal moves", "Black has no legal moves", "threefold repetition",
                  "40-move rule (no capture or man move)", "endgame database adjudication",
                  "adjudicated draw after 400 plies (max plies)", "draw claimed by Cake"):
            self.assertTrue(G.on_board(t), t)
        for t in ("Cake 1.89f lost on time (0.712s used, 0.500s left)", "Regicide 0.2 crashed: engine exited during search",
                  "Kingsrow did not start a new game", "Cake timed out: no reply", "Cake illegal move: 1-5",
                  "Cake protocol error: x", "interrupted", "", None):
            self.assertFalse(G.on_board(t), t)


if __name__ == "__main__":
    unittest.main(verbosity=1)
