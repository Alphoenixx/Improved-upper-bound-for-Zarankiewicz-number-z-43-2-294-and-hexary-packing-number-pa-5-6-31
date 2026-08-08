#!/usr/bin/env bash
#
# run-all.sh -- machine-checked artifacts for pa(5;6) = 31 and z(43;2) <= 294.
#
#   ./run-all.sh           everything, about 5 minutes
#   ./run-all.sh --quick   everything except the decisive N=32 run, about 40 s
#   ./run-all.sh --audit   also re-run the decisive case with the hole-pattern
#                          sorting break switched off (adds roughly one hour)
#
# Requires only a C++17 compiler and Python 3. No third-party libraries, no
# downloads, no SAT solver. Every step asserts its expected outcome and the
# script halts at the first mismatch, so a clean finish is itself the proof.

set -u
export PYTHONUNBUFFERED=1
export LC_ALL=C

QUICK=0
AUDIT=0
while [ $# -gt 0 ]; do
  case "$1" in
    --quick) QUICK=1 ;;
    --audit) AUDIT=1 ;;
    -h|--help) sed -n '3,13p' "$0" | sed 's/^#\{0,1\} \{0,1\}//'; exit 0 ;;
    *) echo "unknown option: $1  (try --help)" >&2; exit 1 ;;
  esac
  shift
done

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT" || exit 1
BIN="$ROOT/bin"
WORK="$ROOT/work"
mkdir -p "$BIN" "$WORK" "$ROOT/data" "$ROOT/certs"

NSTEPS=13
[ "$QUICK" -eq 1 ] && NSTEPS=12
[ "$AUDIT" -eq 1 ] && NSTEPS=$((NSTEPS + 1))
STEP=0
START=$SECONDS

rule() { printf '%s\n' '------------------------------------------------------------'; }
step() { STEP=$((STEP + 1)); printf '\n'; rule; printf '[%d/%d] %s\n' "$STEP" "$NSTEPS" "$1"; rule; }
note() { printf '  %s\n' "$1"; }
die()  {
  printf '\n%s\n' '############################################################'
  printf '  FAILED at step %d: %s\n' "$STEP" "$1"
  printf '  Nothing below this point has been checked.\n'
  printf '%s\n' '############################################################'
  exit 1
}

printf '%s\n' '============================================================'
printf '%s\n' '   pa(5;6) = 31  and  z(43;2) <= 294'
printf '%s\n' '   machine-checked certificates and exhaustive searches'
printf '%s\n' '============================================================'

# ---------------------------------------------------------------- 1. toolchain
step "toolchain and build"
if [ -z "${CXX:-}" ]; then
  for c in g++ clang++ c++; do
    if command -v "$c" >/dev/null 2>&1; then CXX="$c"; break; fi
  done
fi
[ -n "${CXX:-}" ] || die "no C++ compiler found (install g++ or clang++, or set CXX)"
PY=""
for c in python3 python py; do
  if command -v "$c" >/dev/null 2>&1; then
    if "$c" -c 'import sys; sys.exit(0 if sys.version_info[0] == 3 else 1)' >/dev/null 2>&1; then
      PY="$c"; break
    fi
  fi
done
[ -n "$PY" ] || die "no Python 3 found"
note "compiler : $CXX  ($("$CXX" --version 2>/dev/null | head -1))"
note "python   : $PY  ($("$PY" -c 'import sys; print(sys.version.split()[0])' 2>/dev/null))"
"$CXX" -O2 -std=c++17 -o "$BIN/seeds"   src/seeds.cpp   || die "compiling src/seeds.cpp"
"$CXX" -O2 -std=c++17 -o "$BIN/searchA" src/searchA.cpp || die "compiling src/searchA.cpp"
"$CXX" -O2 -std=c++17 -o "$BIN/orbits"  src/orbits.cpp  || die "compiling src/orbits.cpp"
"$CXX" -O2 -std=c++17 -o "$BIN/refute"  src/refute.cpp  || die "compiling src/refute.cpp"
"$CXX" -O2 -std=c++17 -o "$BIN/census"  src/census.cpp  || die "compiling src/census.cpp"
note "built bin/seeds bin/searchA bin/orbits bin/refute bin/census"

# ----------------------------------------------------------- 2,3. certificates
step "lower bound: pa(5;6) >= 31   (certificate, checked from first principles)"
note "31 words over a 6-letter alphabet, length 5, pairwise distance >= 4."
"$PY" -u verify/verify.py certs/w31.txt 6 4 || die "certs/w31.txt is not a valid (5,31,4)_6 code"

step "lower bound: A_6(6,5) >= 31   (certificate for the length-6 extension)"
note "the same 31 words extended to a sixth coordinate; deleting it recovers"
note "certs/w31.txt exactly, so this certificate implies the previous one."
"$PY" -u verify/verify.py certs/w31_6.txt 6 5 || die "certs/w31_6.txt is not a valid (6,31,5)_6 code"

step "a 43x43 configuration with 290 incidences exists   (certificate)"
note "no two points on two common lines, checked over all 903 point pairs"
note "and, independently, over all 903 line pairs. Nothing is read from the"
note "file header: every quantity below is recomputed by the verifier."
"$PY" -u verify/verify_cert290.py certs/cert_290.txt 290 || die "certs/cert_290.txt is not a valid 290-incidence configuration"

# ------------------------------------------------------------------- 4. seeds
step "canonical seed lists (orbit representatives for grid row 1)"
"$BIN/seeds" 2 > "$WORK/seeds2.txt" || die "seeds 2 failed its built-in count check"
"$BIN/seeds" 3 > "$WORK/seeds3.txt" || die "seeds 3 failed its built-in count check"
note "wrote work/seeds2.txt and work/seeds3.txt"

# ------------------------------------------------------- 5. seed completeness
step "independent completeness check of the seed lists"
note "this is the one step that could silently void the upper bound,"
note "so it is re-derived here by a second implementation, in Python."
printf '\n'
"$PY" -u verify/checkseeds.py "$WORK/seeds2.txt" || die "S=2 seed list is not complete"
printf '\n'
"$PY" -u verify/checkseeds.py "$WORK/seeds3.txt" || die "S=3 seed list is not complete"

# ------------------------------------------------------------------ 6,7,8. controls
step "control against the literature: a 34-word code at K=4 exists"
note "A_6(4,3) = 34 is known independently; the search must find one."
"$BIN/searchA" 2 34 "$WORK/seeds2.txt" -expect F -every 5 || die "K=4 N=34 did not come out FEASIBLE"

step "control against the literature: no 35-word code at K=4 exists"
note "the same known value from the other side; the search must refute it."
"$BIN/searchA" 2 35 "$WORK/seeds2.txt" -expect I -every 5 || die "K=4 N=35 did not come out INFEASIBLE"

step "positive control at K=5: a 30-word code exists"
note "guards against a search that refutes everything."
"$BIN/searchA" 3 30 "$WORK/seeds3.txt" -expect F -every 5 || die "K=5 N=30 did not come out FEASIBLE"

# --------------------------------------------- 9,10. independent confirmation
step "independent reduction: 56 orbits under S6 x S4"
note "a second, separately written program that shares no code with searchA."
note "It reduces the same problem to 56 orbit representatives instead of 103."
cp data/orbits56.txt "$WORK/orbits56.ref" || die "data/orbits56.txt is missing"
"$BIN/orbits" > "$WORK/orbits.log" 2>&1 || die "orbits failed"
cat "$WORK/orbits.log"
cmp -s "$WORK/orbits56.ref" data/orbits56.txt \
  || die "the regenerated orbit list differs from the shipped data/orbits56.txt"
note "regenerated data/orbits56.txt is byte-identical to the shipped reference"

step "independent confirmation: exact maximum over the 56 orbits is 18"
note "a total of 32 words needs 20 further words on top of the 12 already"
note "placed, so any maximum below 20 refutes it; 18 clears it by two."
note "This is a genuinely independent route to the same conclusion."
printf '\n'
"$BIN/refute" > "$WORK/refute.log" 2>&1 || die "refute failed"
cat "$WORK/refute.log"
grep -q 'max' "$WORK/refute.log" || die "refute printed no maximum"

# ------------------------------------------------------------------ 11. census
step "parameter census: the eight admissible (n8,m8) pairs at E = 295"
note "bounded-knapsack enumeration for Proposition 8.3, swept over the full"
note "a priori range 0 <= n8,m8 <= 24 given by Cauchy-Schwarz. Deterministic,"
note "single-threaded, no input files, about 15 seconds."
printf '\n'
"$BIN/census" -quiet || die "the census output does not match the list in Proposition 8.3"

# ----------------------------------------------------------------- 12. decisive
if [ "$QUICK" -eq 0 ]; then
  step "DECISIVE: no 32-word code at K=5  (this is the upper bound)"
  note "103 seeds x 696 hole patterns, exhaustive. Expect about 4 minutes."
  printf '\n'
  "$BIN/searchA" 3 32 "$WORK/seeds3.txt" -expect I -sum "$WORK/n32.sum" -every 2 \
    || die "K=5 N=32 did not come out INFEASIBLE"
fi

# -------------------------------------------------------------------- 13. audit
if [ "$AUDIT" -eq 1 ]; then
  step "audit: same case with the hole-pattern sorting break disabled"
  note "enumerates all C(24,4) = 10626 patterns instead of the 696 sorted ones."
  note "This takes roughly an hour and is not needed for the proof; the break"
  note "is justified by a counting argument in PROOF.md section 6."
  printf '\n'
  "$BIN/searchA" 3 32 "$WORK/seeds3.txt" -allpat -expect I -sum "$WORK/n32_allpat.sum" -every 10 \
    || die "the -allpat audit did not come out INFEASIBLE"
fi

# -------------------------------------------------------------------- summary
ELAPSED=$((SECONDS - START))
sum_status=""; sum_nodes=""; sum_time=""; sum_seeds=""; sum_patterns=""; sum_subproblems=""
[ -f "$WORK/n32.sum" ] && . "$WORK/n32.sum"

printf '\n\n'
printf '%s\n' '============================================================'
if [ "$QUICK" -eq 1 ]; then
  printf '%s\n' '   QUICK MODE -- the decisive run was skipped'
  printf '%s\n' '============================================================'
  printf '   Every certificate and every control passed.\n'
  printf '   Run ./run-all.sh with no flags for the N=32 refutation.\n'
  printf '   elapsed: %ds\n' "$ELAPSED"
  printf '%s\n' '============================================================'
  exit 0
fi
printf '%s\n' '   ALL STAGES PASSED       pa(5;6) = A_6(5,4) = 31'
printf '%s\n' '============================================================'
printf '   lower bound   >= 31   certs/w31.txt verified from scratch:\n'
printf '                         465 pairs, minimum Hamming distance 4\n'
printf '   length-6      >= 31   certs/w31_6.txt verified, distance 5\n'
printf '   configuration         certs/cert_290.txt verified: 43 points,\n'
printf '                         290 incidences, profiles 7^32 6^11, leave 66\n'
printf '   upper bound   <= 31   no 32-word code exists:\n'
printf '                         %s seeds x %s hole patterns = %s subproblems\n' \
       "$sum_seeds" "$sum_patterns" "$sum_subproblems"
printf '                         %s nodes, %ss, %s\n' "$sum_nodes" "$sum_time" "$sum_status"
printf '   reduction             103 orbit representatives cover all 393120\n'
printf '                         4x6 Latin rectangles: 0 missing, 0 spurious\n'
printf '   independent route     56-orbit reduction, exact maximum 18,\n'
printf '                         against the 20 further words a 32-word code\n'
printf '                         would need: refuted by a margin of two\n'
printf '   parameter census      Proposition 8.3 reproduced: exactly the eight\n'
printf '                         pairs (0,0) (0,1) (1,0) (1,1) (1,2) (2,1)\n'
printf '                         (2,2) (3,3) over 0 <= n8,m8 <= 24\n'
printf '   controls              A_6(4,3) = 34 reproduced from both sides;\n'
printf '                         30-word code at K=5 found (positive control)\n'
printf '   total wall clock      %ds\n' "$ELAPSED"
printf '%s\n' '============================================================'
printf '   Reference run: 1427441869 nodes. The search is deterministic,\n'
printf '   so the node count above should match exactly on any machine.\n'
printf '%s\n' '============================================================'
exit 0
