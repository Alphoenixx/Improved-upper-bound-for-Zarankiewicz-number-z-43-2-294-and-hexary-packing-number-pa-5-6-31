# What is proved, and by what

This file maps every claim to the artifact that establishes it, and records the
exact numbers a correct run must reproduce. The searches are deterministic and
single-threaded, so node counts are exact fingerprints, not approximations: a
different number means a different program, not a faster machine.

## 1. The two kinds of claim

**Existence** claims need only a certificate. `certs/` holds three objects small
enough to print in an appendix. A referee who distrusts every program in this
repository can still check them, by hand or with ten minutes of their own code.
The verifiers in `verify/` share no code with the search programs and were
written from the definitions.

**Non-existence** claims cannot be certified this way. There is no short witness
for "no 32-word code exists", so the search program itself is the object under
review. That is why `src/searchA.cpp` is instrumented, why its reduction is
re-derived by a second implementation in Python, and why the same conclusion is
reached again by a separately written program using a different group.

## 2. pa(5;6) >= 31

`certs/w31.txt`, checked by `verify/verify.py certs/w31.txt 6 4`.

```
n=5  q=6  N=31  pairs=465
minimum Hamming distance = 4  (required >= 4)
violating pairs = 0
  coordinate 0..4 class profile (sorted): [6, 5, 5, 5, 5, 5]   max=6
RESULT: OK - valid (5,31,4)_6 code
```

All 465 pairs are tested; none agrees in two or more coordinates. The class
profile `(6,5,5,5,5,5)` is the extremal one: exactly one full class of six.
That profile is where the value 31 comes from, by arithmetic rather than by
search, since `6 + 5*5 = 31`.

## 3. A_6(6,5) >= 31

`certs/w31_6.txt`, checked by `verify/verify.py certs/w31_6.txt 6 5`.

The same 31 words with a sixth coordinate appended, pairwise distance now 5.
Deleting the sixth coordinate returns `certs/w31.txt` exactly, so this
certificate implies the previous one. Every one of the six coordinates has
class profile `(6,5,5,5,5,5)`.

## 4. A 43x43 configuration with 290 incidences

`certs/cert_290.txt`, checked by `verify/verify_cert290.py`.

```
points = 43   lines = 43   incidences = 290
violating point pairs = 0   violating line pairs = 0
point degree profile: 7^32 6^11
line  size   profile: 7^32 6^11
point-pairs covered = 837 of 903, leave = 66
line-pairs  covered = 837 of 903, leave = 66
```

The verifier checks the no-two-points-on-two-lines condition twice, once over
all `C(43,2) = 903` point pairs and once, independently, over all 903 line
pairs. It recomputes every quantity from the incidence list and prints it;
nothing is read from the file's header comment, so a stale comment cannot hide
an error. The leave satisfies `903 - (32*21 + 11*15) = 66` on both sides.

## 5. The reduction to 103 classes is complete

`src/seeds.cpp` produces canonical representatives for the first grid row.
`verify/checkseeds.py` re-derives the same reduction independently in Python and
reports `0 missing, 0 spurious`.

| Run | Classes | Out of |
|---|---|---|
| `seeds 2` | 28 | 21 280 |
| `seeds 3` | 103 | 393 120 |
| `seeds 3 noinv` | 134 | 393 120 |

This is the single step that could silently void the upper bound: if the orbit
list were incomplete, the search would be exhaustive over the wrong set and
would still report INFEASIBLE. It is verified complete rather than assumed.
The group has order 8 640, and `393120 / 8640` is not an integer because the
orbits are not all regular, which is exactly why the count has to be checked
rather than divided.

## 6. pa(5;6) <= 31: no 32-word code

`src/searchA.cpp`, run as `searchA 3 32 seeds3.txt -expect I`.

```
S=3 K=5 N=32 seeds[0,103) patterns=696 allpat=0
INFEASIBLE (exhaustive)  nodes=1427441869 time=245.59s
```

Structure of the case split. A `(5,N,4)_6` code is a packing array `PA(N;5,6,2)`,
equivalently three pairwise orthogonal partial Latin squares of order 6. Each
coordinate partitions the words into 6 classes of size at most 6. If no class is
full the code has at most 30 words; with one full class it has at most
`6 + 5*5 = 31`; with two or more full classes the bound is `6 + 6 + 18 = 30`.
So `N = 32` forces at least two full classes, which is what the search refutes.

With rows 0 and 1 full there are `H = 36 - N` holes in rows 2 to 5, giving
`C(24,4) = 10 626` hole patterns, which reduce to **696** under sorting:

```
15 + 120 + 120 + 315 + 126 = 696
[4]  [3,1] [2,2] [2,1,1] [1^4]
```

so `103 x 696 = 71 688` subproblems.

## 7. Controls

| Run | Required verdict | Nodes | Purpose |
|---|---|---|---|
| `searchA 2 34` | FEASIBLE | 159 | reproduces published `A_6(4,3) = 34` from below |
| `searchA 2 35` | INFEASIBLE | 7 234 770 | reproduces the same value from above |
| `searchA 3 30` | FEASIBLE | 19 | the search is not refuting everything |
| `searchA 3 33` | INFEASIBLE | 105 161 035 | cheaper instance of the decisive shape |
| `searchA 3 32 -allpat` | INFEASIBLE | audit | drops the sorting break, all 10 626 patterns |
| `seeds 3 noinv` + `searchA 3 32` | INFEASIBLE | 1 802 714 985 | drops the row-swap break, 134 seeds |

The first two are the ones with real force: they recover a value established
independently in the literature, from both directions, using the same code path
as the decisive run. The two audits remove one symmetry break each and re-run;
both reach the same verdict at higher cost, which is what a sound break must do.

A target of 33 was also checked and comes out INFEASIBLE, but that is a
consistency check only. It is implied by the 32 result and carries no
independent evidence.

## 8. The independent 56-orbit route

`src/orbits.cpp` and `src/refute.cpp` are a second implementation that shares no
code with `searchA`. They reduce the same question under `S6 x S4` (order
17 280) to 661 orbits under `S6` and then 56 orbits, and compute the exact
maximum over all of them:

```
19 837 905 nodes, maximum = 18, about 4 seconds
```

A 32-word code would need 20 further words on top of the 12 already placed, so
any maximum below 20 refutes it; 18 clears it by two. Per-orbit
maxima are 16 for one representative, 17 for eight of them, and 18 for the rest.
The run regenerates `data/orbits56.txt` and the script requires it to be
byte-identical to the shipped copy.

The two routes use different groups, different reductions and different search
code, and agree.

## 9. Exit codes

`searchA` exits `0` normally, `1` on usage or I/O error, `3` when the verdict
contradicts `-expect`, and `4` on timeout. `census` exits `0` when its output
matches the paper, `1` on usage or a failed internal sanity check, and `3` on a
mismatch. `run-all.sh` and `run-all.ps1` assert
the expected verdict at every step and stop at the first mismatch, so reaching
the final banner is the whole check.

## 10. The parameter census (Proposition 8.3)

`src/census.cpp`, run as `bin/census`. Deterministic, single-threaded, no input
files, no randomness, about 12 seconds.

```
a priori: 0 <= n8,m8 <= 24 (Cauchy-Schwarz)

(n8,m8) pairs admitting a joint solution of (I) and (II):
  (0,0)
  (0,1)
  (1,0)
  (1,1)
  (1,2)
  (2,1)
  (2,2)
  (3,3)

paper claims exactly: (0,0) (0,1) (1,0) (1,1) (1,2) (2,1) (2,2) (3,3)
computed count = 8, claimed count = 8
MATCH: YES -- Proposition 8.3 is CONFIRMED
```

The sweep range is not assumed, it is re-derived. The short lines carry total
weight `6 + m_8`, there are at most `43 - m_8` of them, and `sum w^2 <= 78 - m_8`,
so Cauchy-Schwarz gives `(6+m_8)^2 <= (43-m_8)(78-m_8)`, that is `133 m_8 <= 3318`
and `m_8 <= 24`. The program asserts both that every `b <= 24` satisfies that
inequality and that `b = 25` does not, and only then sweeps `0 <= n_8, m_8 <= 24`.

For each parameter `a` it builds, by dynamic programming over item weights
`w = 1..7`, the Pareto frontier of achievable pairs `(sum w^2, max G)` subject to
the count, packing and leave constraints, covering every budget `b` in one pass.
A pair `(n_8, m_8)` survives when some line-side frontier point and some
point-side frontier point together satisfy the coupling inequalities (I) and (II).

Exit `0` means the computed list matches the eight pairs printed in the paper;
exit `3` prints the symmetric difference. What needs the sweep is exhaustiveness:
that each of the eight pairs is reachable follows from a witness selection given
in the paper.

## 11. What this repository does not cover

The route to `z(43;2) <= 294` also uses steps that are hand-checkable and need
no code: the deficiency dictionary, the counting bounds of the cleanup section,
the coupling inequalities, and the projective-plane identification of the last
surviving case.
