# pa(5;6) = 31 and z(43;2) <= 294

Machine-checkable artifacts for

> Ankan Sadhu. *An improved upper bound for the Zarankiewicz number
> z(43;2) <= 294 via structural rigidity, including the hexary packing number
> pa(5;6) = 31.* 2026.

C++17 and Python 3, standard libraries only. No third-party packages, no
downloads, no SAT solver, no build system to install. One script runs
everything and asserts every expected verdict, so a clean finish is itself the
check.

## Run it

Linux / macOS:

```sh
./run-all.sh            # everything, about 5 minutes
./run-all.sh --quick    # skip the decisive 4-minute run, about 40 seconds
./run-all.sh --audit    # add the symmetry-break audit, roughly one hour
```

Windows (PowerShell 5.1 or 7+):

```powershell
powershell -ExecutionPolicy Bypass -File .\run-all.ps1
powershell -ExecutionPolicy Bypass -File .\run-all.ps1 -Quick
```

If Windows refuses the script because it arrived in a zip, use the
`-ExecutionPolicy Bypass` form above, or run `Unblock-File .\run-all.ps1`
first. Nothing here needs administrator rights.

Output is unbuffered. The long search prints a progress line every two seconds
with percentage, current seed, node count, rate and ETA, then a one-line
verdict.

## What proves what

| Claim in the paper | Artifact | Kind | Time |
|---|---|---|---|
| `pa(5;6) >= 31` | `certs/w31.txt` + `verify/verify.py` | certificate | < 1 s |
| `A_6(6,5) >= 31` | `certs/w31_6.txt` + `verify/verify.py` | certificate | < 1 s |
| a 43x43 configuration with 290 incidences exists | `certs/cert_290.txt` + `verify/verify_cert290.py` | certificate | ~2 s |
| the reduction to 103 classes is complete | `src/seeds.cpp` + `verify/checkseeds.py` | exhaustive, two implementations | ~2 s |
| `A_6(4,3) = 34` (control, known value) | `src/searchA.cpp` | exhaustive search | ~1 s |
| `pa(5;6) <= 31`: no 32-word code | `src/searchA.cpp` | exhaustive search | ~245 s |
| independent confirmation, 56 orbits | `src/orbits.cpp`, `src/refute.cpp` | exhaustive search | ~5 s |
| the parameter census of Proposition 8.3 | `src/census.cpp` | exhaustive enumeration | ~12 s |

The first three rows are **certificates**. A referee does not have to trust any
program here: the objects are small enough to print, and a replacement verifier
takes ten minutes to write. The certificate is the proof; `verify/` is a
convenience.

The remaining rows are **non-existence** claims. There is no certificate for
"no such object exists", so the search program is the only thing that can be
handed over, which is why it is here and why it is instrumented.

## Why the controls matter

A search that refutes everything refutes nothing. Before the decisive run the
script requires all of these:

| Input | Required verdict | What it would catch |
|---|---|---|
| `searchA 2 34` | FEASIBLE (159 nodes) | reproduces the published `A_6(4,3) = 34` from below |
| `searchA 2 35` | INFEASIBLE (7 234 770 nodes) | reproduces the same published value from above |
| `searchA 3 30` | FEASIBLE (19 nodes) | a search too aggressive to find anything |
| `checkseeds.py` | 0 missing, 0 spurious | the orbit reduction quietly dropping cases |
| `refute` over 56 orbits | maximum 18, against the 20 needed | a bug shared between reduction and search |

The last two are the ones that actually protect the upper bound. The 103-class
reduction is re-derived in Python by a second implementation, and the whole
question is then re-answered by a separately written program using a different
reduction (56 orbits under `S6 x S4` rather than 103 under a group of order
8 640). The two routes agree.

## Layout

```
src/seeds.cpp          canonical 4x6 Latin rectangle representatives (103 for K=5)
src/searchA.cpp        the exhaustive search; CLI documented below
src/orbits.cpp         independent reduction to 56 orbits under S6 x S4
src/refute.cpp         exact maximum over those 56 orbits
src/census.cpp         parameter census of Prop 8.3: the eight (n8,m8) pairs
src/tails.hpp          shared tail enumeration for orbits/refute
verify/verify.py       q-ary code verifier, first principles, no shared code
verify/checkseeds.py   independent completeness check of the seed lists
verify/verify_cert290.py  incidence-configuration verifier
certs/w31.txt          31 words, length 5, alphabet 6, distance 4
certs/w31_6.txt        the same 31 words extended to length 6, distance 5
certs/cert_290.txt     43 points, 290 incidences, degree profile 7^32 6^11
data/orbits56.txt      reference orbit list, regenerated and compared on each run
run-all.sh run-all.ps1 the two entry points
PROOF.md               claim-by-claim map with the exact expected numbers
```

`bin/` and `work/` are created at run time and are not tracked, and neither are
`data/orbits661.txt` and `certs/refute.cert`, which `orbits` and `refute`
regenerate. `data/orbits56.txt` *is* tracked, because the run regenerates it and
then requires the result to be byte-identical to the shipped copy.

## searchA command line

```
searchA S N seedfile [-expect F|I] [-sum FILE] [-every SECS]
                     [-quiet] [-allpat] [-lo i] [-hi j] [-t SECS]
```

`S` is the number of squares (`S=2` is length 4, `S=3` is length 5), `N` the
target number of words. Exit codes: `0` normal, `1` usage or I/O error,
`3` the verdict contradicts `-expect`, `4` timed out. `-sum FILE` writes a
machine-readable summary (`sum_status`, `sum_nodes`, `sum_time`, ...).

The search is deterministic and single-threaded, so node counts are exact
fingerprints: any machine must reproduce `1427441869` for the decisive run.

## What is deliberately not here

The paper's route to `z(43;2) <= 294` also uses several steps that are
hand-checkable and therefore need no code: the deficiency dictionary, the
counting bounds in the cleanup section, the projective-plane identification,
and the arithmetic that produces the value 31 from the class-profile ceiling
(`6 + 5*5 = 31`). The exhaustive search does not establish 31; it rules out the
competing two-full-class configuration, which turns out to fail by two words.

Everything else the paper computes is here, including the parameter census of
section 9: that is the bounded-knapsack enumeration in `src/census.cpp`, which
reproduces the eight admissible `(n8, m8)` pairs of Proposition 8.3 over the full
a priori range `0 <= n8, m8 <= 24` and exits non-zero if its list differs from the
one printed in the paper.


