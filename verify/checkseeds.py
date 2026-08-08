#!/usr/bin/env python3
"""Independent completeness check for the canonical seed list.

The upper bound in this repository rests on one reduction step: after pinning
grid rows 0 and 1, row 1 becomes an ordered S-tuple of pairwise discordant
derangements of [6] (equivalently an (S+1) x 6 Latin rectangle whose first row
is the identity), and the search only visits ONE representative per symmetry
orbit.  If that representative list misses even a single orbit, the search is
not exhaustive and every INFEASIBLE verdict it produces is void.

This script re-derives that statement from scratch, in Python, without looking
at seeds.cpp:

  1. build every ordered S-tuple of pairwise discordant derangements of [6];
  2. expand each listed seed over the full group the normalisation is allowed
     to use:
         (a) p_s -> g p_s g^-1  for all g in S_6   (column + symbol relabelling)
         (b) any permutation of the S squares
         (c) p_s -> p_s^-1                          (swap grid rows 0 and 1)
  3. check that the union of those orbits equals the set from step 1 exactly:
     nothing uncovered, nothing outside it.

usage: checkseeds.py <seedfile>
exit:  0 complete and sound, 1 otherwise
"""
import sys
import time
from itertools import permutations

LITERATURE = {1: 265, 2: 21280, 3: 393120}


def inverse(p):
    q = [0] * 6
    for i, v in enumerate(p):
        q[v] = i
    return tuple(q)


def main():
    if len(sys.argv) < 2:
        print("usage: checkseeds.py <seedfile>")
        return 1
    path = sys.argv[1]

    S = None
    inv = True
    hdr_seeds = None
    hdr_tuples = None
    seeds = []
    for raw in open(path):
        line = raw.strip()
        if line.startswith("#"):
            for tok in line[1:].split():
                if "=" in tok:
                    k, v = tok.split("=", 1)
                    if k == "S":
                        S = int(v)
                    elif k == "inv":
                        inv = (v == "1")
                    elif k == "seeds":
                        hdr_seeds = int(v)
                    elif k == "tuples":
                        hdr_tuples = int(v)
            continue
        if not line:
            continue
        seeds.append(tuple(tuple(int(c) for c in t) for t in line.split()))

    if not seeds:
        print("FAIL: no seeds found in %s" % path)
        return 1
    if S is None:
        S = len(seeds[0])
    for sd in seeds:
        if len(sd) != S or any(sorted(p) != list(range(6)) for p in sd):
            print("FAIL: malformed seed %s" % (sd,))
            return 1
    if hdr_seeds is not None and hdr_seeds != len(seeds):
        print("FAIL: header says seeds=%d but %d rows present" % (hdr_seeds, len(seeds)))
        return 1

    print("  seed file                    : %s" % path)
    print("  squares S                    : %d" % S)
    print("  representatives listed       : %d" % len(seeds))
    print("  inversion used in the group  : %s" % ("yes" if inv else "no"))

    t0 = time.time()

    P = list(permutations(range(6)))
    D = [p for p in P if all(p[i] != i for i in range(6))]
    print("  derangements of [6]          : %d   (expected 265)" % len(D))
    if len(D) != 265:
        print("FAIL: wrong number of derangements")
        return 1

    n = len(D)
    adj = [0] * n
    for i in range(n):
        pi = D[i]
        m = 0
        for j in range(n):
            pj = D[j]
            good = True
            for y in range(6):
                if pi[y] == pj[y]:
                    good = False
                    break
            if good:
                m |= 1 << j
        adj[i] = m

    full = set()
    prefix = []

    def rec(mask):
        if len(prefix) == S:
            full.add(tuple(D[i] for i in prefix))
            return
        m = mask
        while m:
            low = m & -m
            j = low.bit_length() - 1
            m -= low
            prefix.append(j)
            rec(mask & adj[j])
            prefix.pop()

    rec((1 << n) - 1)
    t1 = time.time()
    print("  all ordered %d-tuples built    : %d   (%.1f s)" % (S, len(full), t1 - t0))
    if S in LITERATURE:
        print("  %dx6 Latin rectangles, row0=id : %d   (expected %d)"
              % (S + 1, len(full), LITERATURE[S]))
        if len(full) != LITERATURE[S]:
            print("FAIL: rectangle count disagrees with the classical value")
            return 1
    if hdr_tuples is not None and hdr_tuples != len(full):
        print("FAIL: header says tuples=%d but %d were built here" % (hdr_tuples, len(full)))
        return 1

    for sd in seeds:
        if sd not in full:
            print("FAIL: listed seed is not a valid Latin rectangle: %s" % (sd,))
            return 1

    sqperms = list(permutations(range(S)))
    union = set()
    for sd in seeds:
        variants = [sd]
        if inv:
            variants.append(tuple(inverse(p) for p in sd))
        for base in variants:
            for sp in sqperms:
                b2 = [base[k] for k in sp]
                for g in P:
                    out = []
                    for p in b2:
                        r = [0] * 6
                        for i in range(6):
                            r[g[i]] = g[p[i]]
                        out.append(tuple(r))
                    union.add(tuple(out))
    t2 = time.time()

    missing = full - union
    spurious = union - full
    print("  union of the %3d orbits       : %d   (%.1f s)" % (len(seeds), len(union), t2 - t1))
    print("  rectangles NOT covered       : %d" % len(missing))
    print("  images outside the set       : %d" % len(spurious))

    if missing:
        for x in list(missing)[:3]:
            print("    uncovered example: %s" % (x,))
    if spurious:
        for x in list(spurious)[:3]:
            print("    spurious example : %s" % (x,))

    if missing or spurious:
        print("RESULT: FAIL - the seed list is not a complete set of orbit representatives")
        return 1

    print("RESULT: OK - the %d representatives cover all %d rectangles exactly"
          % (len(seeds), len(full)))
    print("        the two-full-rows reduction loses nothing (%.1f s total)" % (t2 - t0))
    return 0


sys.exit(main())
