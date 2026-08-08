# A_6(5,4) = 31

Everything needed to check, from scratch, that the largest code of length 5 over
a 6-letter alphabet with minimum Hamming distance 4 has exactly **31** words --
and nothing else.

Equivalently: `pa(5;6) = 31`, the largest packing array `PA(N;5,6,2)` has
`N = 31`, the largest partial `TD(5,6)` has 31 blocks, and three pairwise
orthogonal partial Latin squares of order 6 fill at most 31 of the 36 cells.

## Run it

```
./run-all.sh          # Linux / macOS / WSL / MSYS2        about 4-5 minutes
.\run-all.ps1         # Windows PowerShell 5.1 or 7+       about 4-5 minutes
```

| flag | effect |
|---|---|
| `--quick` / `-Quick` | everything except the decisive run, about 15 s -- use this first to confirm the toolchain |
| `--audit` / `-Audit` | adds a redundant re-run with one symmetry break disabled, about 1 hour, not needed for the proof |

Both scripts are unbuffered, print a live progress line every 2 seconds during
the long run, **assert the expected outcome of every step**, and stop at the
first mismatch. A clean finish is the proof; there is no output to interpret.

## Dependencies

A C++17 compiler and Python 3. That is the entire list. No libraries, no
`pip install`, no SAT solver, no network access, nothing vendored, nothing to
download. The scripts locate `g++` / `clang++` / `cl` and `python` / `python3` /
`py` themselves.

## Files

```
run-all.sh            proof driver, POSIX shell
run-all.ps1           proof driver, PowerShell
PROOF.md              the mathematical argument, end to end
src/seeds.cpp         canonical orbit representatives for grid row 1
src/searchA.cpp       the exhaustive search
verify/checkseeds.py  independent completeness check of the seed list
verify/verify.py      independent checker for the 31-word certificate
certs/w31.txt         the 31-word code
```

Nine files. `bin/` and `work/` are created by the scripts and can be deleted.

## What the run does

| step | claim it establishes |
|---|---|
| 1 | toolchain found, both programs compile |
| 2 | seed lists generated; built-in count checks pass |
| 3 | the 103 representatives cover **all** 393120 4x6 Latin rectangles, 0 missing, 0 spurious -- re-derived in Python, independently of `seeds.cpp` |
| 4 | `certs/w31.txt` is a genuine 31-word code: all 465 pairs checked, minimum distance 4, so `A_6(5,4) >= 31` |
| 5 | control: a 34-word code at `K=4` is found -- `A_6(4,3) = 34` is known, so the search is not over-pruning |
| 6 | control: no 35-word code at `K=4` -- the same known value from the other side, so the search is not under-pruning |
| 7 | positive control: a 30-word code at `K=5` is found |
| 8 | **decisive**: no 32-word code at `K=5` exists, so `A_6(5,4) <= 31` |

Steps 4 and 8 together give `A_6(5,4) = 31`. Steps 3, 5, 6 and 7 are what make
step 8 believable.

## Expected numbers

The search is deterministic -- no randomness, no restarts, no time-dependent
heuristics -- so node counts reproduce exactly on any machine. Times are for one
core of a 2.9 GHz Xeon.

| run | verdict | nodes | time |
|---|---|---|---|
| `seeds 2` | 28 seeds from 21280 rectangles | | <1 s |
| `seeds 3` | 103 seeds from 393120 rectangles | | <1 s |
| `checkseeds.py seeds3.txt` | 0 uncovered, 0 spurious | | ~4 s |
| `verify.py certs/w31.txt 6 4` | valid `(5,31,4)_6` code | | <1 s |
| `searchA 2 34` | FEASIBLE | 159 | <0.1 s |
| `searchA 2 35` | INFEASIBLE | 7234770 | ~1.2 s |
| `searchA 3 30` | FEASIBLE | 19 | <0.1 s |
| `searchA 3 32` | **INFEASIBLE (exhaustive)** | **1427441869** | ~250 s |

If your node counts differ from these, something is wrong -- report it rather
than trusting the verdict.

## Where 31 comes from

Not from the search. Each class of a coordinate has at most 6 words, so a code
with at most one full class in some coordinate has at most `6 + 5*5 = 31` words.
That single line of arithmetic is the bound. The search exists only to rule out
the remaining case -- a 32-word code, which must then have **two** full classes
-- and it does so exhaustively. See `PROOF.md` sections 3 and 4.

## Running pieces by hand

```
bin/seeds 3 > work/seeds3.txt
bin/searchA 3 32 work/seeds3.txt -expect I -every 2
bin/searchA 3 32 work/seeds3.txt -lo 0 -hi 20        # split across machines
bin/searchA 3 32 work/seeds3.txt -allpat             # without the pattern break
bin/searchA 3 33 work/seeds3.txt -expect I           # ~21 s, monotonicity check
```

`searchA` exit codes: `0` completed and matched `-expect`, `1` usage or I/O
error, `3` the verdict contradicted `-expect`, `4` hit a `-t` time limit without
reaching a verdict. A timeout is never reported as a verdict.

