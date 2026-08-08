#!/usr/bin/env python3
"""Independent verifier for the 290-incidence configuration (Appendix A).

Reads a point-line incidence list and checks, from first principles:
  * every line index is in range and no line repeats within a point,
  * the total number of incidences,
  * no two points lie on two common lines (equivalently: the 43x43 zero-one
    matrix has no all-one 2x2 submatrix), checked directly over all C(43,2)
    point pairs AND, independently, over all C(43,2) line pairs,
  * the point-degree and line-size profiles,
  * the uncovered-pair count (the "leave") on each side.

Nothing is taken from the file's header: every quantity is recomputed here and
printed, so a stale or wrong comment in the certificate cannot hide an error.

Usage:  verify_cert290.py certs/cert_290.txt [expected_incidences]
"""
import sys
from itertools import combinations


def load(path):
    pts = {}
    for raw in open(path):
        line = raw.split('#')[0].strip()
        if not line:
            continue
        if ':' not in line:
            raise SystemExit('bad line (no colon): %r' % raw)
        head, tail = line.split(':', 1)
        p = int(head.strip())
        ls = [int(t) for t in tail.split()]
        if p in pts:
            raise SystemExit('point %d listed twice' % p)
        pts[p] = ls
    return pts


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'certs/cert_290.txt'
    want = int(sys.argv[2]) if len(sys.argv) > 2 else 290
    pts = load(path)

    ids = sorted(pts)
    n = len(ids)
    if ids != list(range(1, n + 1)):
        raise SystemExit('points must be numbered 1..%d, got %r' % (n, ids[:5]))

    bad = 0
    for p in ids:
        ls = pts[p]
        if len(set(ls)) != len(ls):
            print('  point %d repeats a line' % p); bad += 1
        for l in ls:
            if not (1 <= l <= n):
                print('  point %d has out-of-range line %d' % (p, l)); bad += 1

    inc = sum(len(v) for v in pts.values())
    print('  points = %d   lines = %d   incidences = %d  (expected %d)'
          % (n, n, inc, want))

    # dual incidence structure
    lines = {l: [] for l in range(1, n + 1)}
    for p in ids:
        for l in pts[p]:
            lines[l].append(p)

    # no two points on two common lines
    viol_p = 0
    sets = {p: set(pts[p]) for p in ids}
    for a, b in combinations(ids, 2):
        if len(sets[a] & sets[b]) >= 2:
            viol_p += 1
            if viol_p <= 5:
                print('  points %d,%d share lines %s'
                      % (a, b, sorted(sets[a] & sets[b])))
    # dually, no two lines meeting twice
    viol_l = 0
    lsets = {l: set(lines[l]) for l in lines}
    for a, b in combinations(sorted(lines), 2):
        if len(lsets[a] & lsets[b]) >= 2:
            viol_l += 1
            if viol_l <= 5:
                print('  lines %d,%d share points %s'
                      % (a, b, sorted(lsets[a] & lsets[b])))
    print('  violating point pairs = %d   violating line pairs = %d'
          % (viol_p, viol_l))

    def profile(d):
        from collections import Counter
        c = Counter(len(v) for v in d.values())
        return ' '.join('%d^%d' % (k, c[k]) for k in sorted(c, reverse=True))

    print('  point degree profile: %s' % profile(pts))
    print('  line  size   profile: %s' % profile(lines))

    total = n * (n - 1) // 2
    covp = sum(len(v) * (len(v) - 1) // 2 for v in pts.values())
    covl = sum(len(v) * (len(v) - 1) // 2 for v in lines.values())
    print('  point-pairs covered = %d of %d, leave = %d' % (covl, total, total - covl))
    print('  line-pairs  covered = %d of %d, leave = %d' % (covp, total, total - covp))

    ok = (bad == 0 and viol_p == 0 and viol_l == 0 and inc == want)
    print('RESULT: %s' % ('OK - valid configuration with %d incidences' % inc
                          if ok else 'FAILED'))
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
