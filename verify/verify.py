#!/usr/bin/env python3
"""Independent verifier for q-ary codes / packing arrays / partial transversal designs.

Reads codewords from a file: any whitespace-separated tokens that are strings of
digits (e.g. 00000) or lines of space separated integers (e.g. '0 1 2 3 4').
Lines starting with # are ignored.

Checks, from first principles:
  * every pair of codewords agrees in at most (n - d) coordinates,
  * i.e. minimum Hamming distance >= d,
  * reports the class profile in every coordinate.
"""
import sys
from itertools import combinations


def load(path):
    words = []
    for raw in open(path):
        line = raw.split('#')[0].strip()
        if not line:
            continue
        toks = line.split()
        if all(len(t) > 1 and t.isdigit() for t in toks):
            for t in toks:
                words.append(tuple(int(c) for c in t))
        else:
            nums = [int(t) for t in toks if t.lstrip('-').isdigit()]
            if nums:
                words.append(tuple(nums))
    return words


def main():
    path = sys.argv[1]
    q = int(sys.argv[2]) if len(sys.argv) > 2 else 6
    dmin_req = int(sys.argv[3]) if len(sys.argv) > 3 else 4
    words = load(path)
    if not words:
        print('NO CODEWORDS')
        sys.exit(1)
    n = len(words[0])
    ok = True
    for w in words:
        if len(w) != n:
            print('RAGGED length', w)
            ok = False
        if any(not (0 <= c < q) for c in w):
            print('ALPHABET violation', w)
            ok = False
    if len(set(words)) != len(words):
        print('DUPLICATE codewords')
        ok = False
    worst = n
    bad = 0
    for a, b in combinations(words, 2):
        d = sum(1 for i in range(n) if a[i] != b[i])
        worst = min(worst, d)
        if d < dmin_req:
            bad += 1
            if bad <= 5:
                print('VIOLATION d=%d' % d, a, b)
    print('n=%d  q=%d  N=%d  pairs=%d' % (n, q, len(words), len(words) * (len(words) - 1) // 2))
    print('minimum Hamming distance = %d  (required >= %d)' % (worst, dmin_req))
    print('violating pairs = %d' % bad)
    for i in range(n):
        prof = [0] * q
        for w in words:
            prof[w[i]] += 1
        print('  coordinate %d class profile (sorted): %s   max=%d' %
              (i, sorted(prof, reverse=True), max(prof)))
    print('RESULT:', 'OK - valid (%d,%d,%d)_%d code' % (n, len(words), worst, q)
          if ok and bad == 0 else 'FAIL')
    sys.exit(0 if (ok and bad == 0) else 1)


main()
