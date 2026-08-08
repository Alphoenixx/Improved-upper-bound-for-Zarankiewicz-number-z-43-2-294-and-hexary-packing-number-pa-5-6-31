// refute.cpp -- exact maximum over classes 2..5, for each of the 56 orbits.
//
// Given class 0 = diagonal and class 1 = a full class (one of the 56 orbit
// representatives), the remaining four classes 2..5 draw their tails from
//     pool = { rainbow tails t : ag(t, c) <= 1 for every tail c of class 1 }.
// Inside one class tails must pairwise agree in 0 coordinates; across two
// classes they must agree in at most 1.  A total of >= 32 words needs
// >= 20 words from classes 2..5.
//
// This computes the EXACT maximum, not just the >= 20 decision, so the output
// carries a witness for the lower bound as well as the refutation.
//
// Two things make this fast, neither of which the original program used:
//
//  1. Class symmetry.  Classes 2..5 are interchangeable: nothing in the
//     constraints depends on which first coordinate a class carries.  So we
//     enumerate UNORDERED collections, ordering classes by size descending and
//     breaking ties on least tail index ascending.  Each collection is visited
//     exactly once instead of up to 4! = 24 times.
//
//  2. A size argument.  sum s_i >= 20 with every s_i <= 6 forces the two
//     largest classes to have size >= 5: if the second largest were <= 4 then
//     the total would be at most 6 + 4 + 4 + 4 = 18.  Combined with the
//     non-increasing order this prunes the top of the tree immediately.
#include "tails.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <array>
#include <algorithm>
#include <ctime>

using namespace std;
static pa::Tails T;

// ---------------- 128-bit set over pool indices ----------------------
struct BS { uint64_t w[2]; };
static inline BS bs_and(BS a, BS b) { return BS{{a.w[0] & b.w[0], a.w[1] & b.w[1]}}; }
static inline int  bs_pop(BS a) { return __builtin_popcountll(a.w[0]) + __builtin_popcountll(a.w[1]); }
static inline bool bs_get(BS a, int i) { return (a.w[i >> 6] >> (i & 63)) & 1ULL; }
static inline void bs_clr(BS& a, int i) { a.w[i >> 6] &= ~(1ULL << (i & 63)); }
static inline void bs_set(BS& a, int i) { a.w[i >> 6] |= 1ULL << (i & 63); }
static inline BS bs_from(BS a, int start) {   // drop bits below `start`
    if (start <= 0) return a;
    if (start >= 128) return BS{{0, 0}};
    BS r = a;
    if (start < 64) r.w[0] &= (~0ULL) << start;
    else { r.w[0] = 0; r.w[1] &= (start == 64) ? ~0ULL : ((~0ULL) << (start - 64)); }
    return r;
}

static int NP;
static BS SAME[128], CROSS[128];
static int POOL[128];

static int best;
static int curCfg[4][6], curSz[4];
static int bestCfg[4][6], bestSz[4];
static long long nodes;

static void saveBest(int depth, int total) {
    best = total;
    for (int d = 0; d < 4; d++) {
        bestSz[d] = (d < depth) ? curSz[d] : 0;
        for (int i = 0; i < bestSz[d]; i++) bestCfg[d][i] = curCfg[d][i];
    }
}

static void rec(int depth, BS avail, int prevSize, int prevMin, int total);

static void enumClique(int need, BS cand, int start, int nchosen,
                       int depth, BS availOuter, int total, int s) {
    if (need == 0) {
        BS na = availOuter;
        for (int i = 0; i < s; i++) na = bs_and(na, CROSS[curCfg[depth][i]]);
        for (int i = 0; i < s; i++) bs_clr(na, curCfg[depth][i]);
        curSz[depth] = s;
        rec(depth + 1, na, s, curCfg[depth][0], total + s);
        return;
    }
    BS c2 = bs_from(cand, start);
    if (bs_pop(c2) < need) return;
    for (int wi = 0; wi < 2; wi++) {
        uint64_t m = c2.w[wi];
        while (m) {
            int b = __builtin_ctzll(m);
            m &= m - 1;
            int v = wi * 64 + b;
            curCfg[depth][nchosen] = v;
            enumClique(need - 1, bs_and(cand, SAME[v]), v + 1, nchosen + 1,
                       depth, availOuter, total, s);
        }
    }
}

static void rec(int depth, BS avail, int prevSize, int prevMin, int total) {
    nodes++;
    if (total > best) saveBest(depth, total);
    if (depth == 4) return;
    int left = 4 - depth;
    int cnt  = bs_pop(avail);
    int cap  = left * prevSize;
    if (cnt < cap) cap = cnt;
    if (total + cap <= best) return;                 // cannot beat the incumbent

    int smax = prevSize < 6 ? prevSize : 6;
    for (int s = smax; s >= 1; s--) {
        if (total + s * left <= best) break;         // sizes only get smaller
        int start = (s == prevSize) ? prevMin + 1 : 0;
        enumClique(s, avail, start, 0, depth, avail, total, s);
    }
}

int main() {
    clock_t t0 = clock();

    // ---- read the 56 orbit representatives --------------------------
    FILE* f = fopen("data/orbits56.txt", "r");
    if (!f) { printf("FATAL: data/orbits56.txt missing -- run bin/orbits first\n"); return 1; }
    vector<array<int, 6>> reps;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        array<int, 6> cls{};
        int got = 0;
        for (char* p = strtok(line, " \t\r\n"); p && got < 6; p = strtok(nullptr, " \t\r\n")) {
            if (strlen(p) != 4) { printf("FATAL: bad tail token '%s'\n", p); return 1; }
            int a = p[0] - '0', b = p[1] - '0', c = p[2] - '0', d = p[3] - '0';
            int id = T.idx[T.code(a, b, c, d)];
            if (id < 0) { printf("FATAL: '%s' is not a rainbow tail\n", p); return 1; }
            cls[got++] = id;
        }
        if (got != 6) { printf("FATAL: class with %d tails\n", got); return 1; }
        reps.push_back(cls);
    }
    fclose(f);
    printf("orbit representatives read    : %zu\n", reps.size());

    FILE* out = fopen("certs/refute.cert", "w");
    if (!out) { printf("FATAL: cannot write certs/refute.cert\n"); return 1; }
    fprintf(out, "# Exact maximum number of words in classes 2..5, per orbit.\n");
    fprintf(out, "# Class 0 is the diagonal (6 words); class 1 is the listed full class (6 words).\n");
    fprintf(out, "# A total of 32 would need max_2_5 >= 20.\n");
    fprintf(out, "# Fields: rep, class1 tails, pool size, pool tails, max_2_5, witness.\n\n");

    int globalMax = 0, minPool = 1 << 30, maxPool = 0;
    long long sumPool = 0, totalNodes = 0;

    for (size_t ri = 0; ri < reps.size(); ri++) {
        const auto& cls = reps[ri];

        // sanity: representative is a full class
        for (int a = 0; a < 6; a++)
            for (int b = a + 1; b < 6; b++)
                if (T.ag[cls[a]][cls[b]] != 0) { printf("FATAL: rep %zu not a full class\n", ri + 1); return 1; }

        // pool
        NP = 0;
        for (int t = 0; t < T.n; t++) {
            bool ok = true;
            for (int j = 0; j < 6; j++) if (T.ag[t][cls[j]] > 1) { ok = false; break; }
            if (ok) POOL[NP++] = t;
        }
        if (NP > 128) { printf("FATAL: pool larger than 128\n"); return 1; }
        if (NP < minPool) minPool = NP;
        if (NP > maxPool) maxPool = NP;
        sumPool += NP;

        for (int i = 0; i < NP; i++) { SAME[i] = BS{{0, 0}}; CROSS[i] = BS{{0, 0}}; }
        for (int i = 0; i < NP; i++)
            for (int j = 0; j < NP; j++) {
                if (i == j) continue;
                int s = T.ag[POOL[i]][POOL[j]];
                if (s <= 1) bs_set(CROSS[i], j);
                if (s == 0) bs_set(SAME[i], j);
            }

        BS all = BS{{0, 0}};
        for (int i = 0; i < NP; i++) bs_set(all, i);

        best = 0; nodes = 0;
        memset(bestSz, 0, sizeof(bestSz));
        rec(0, all, 6, -1, 0);
        totalNodes += nodes;
        if (best > globalMax) globalMax = best;

        printf("rep %2zu/%zu  pool=%3d  max(classes 2..5) = %2d  -> total %2d   [%lld nodes]\n",
               ri + 1, reps.size(), NP, best, 12 + best, nodes);
        fflush(stdout);

        fprintf(out, "rep %zu\n", ri + 1);
        fprintf(out, "  class1:");
        for (int j = 0; j < 6; j++) {
            const auto& x = T.t[cls[j]];
            fprintf(out, " %d%d%d%d", x[0], x[1], x[2], x[3]);
        }
        fprintf(out, "\n  pool %d:", NP);
        for (int i = 0; i < NP; i++) {
            const auto& x = T.t[POOL[i]];
            fprintf(out, " %d%d%d%d", x[0], x[1], x[2], x[3]);
        }
        fprintf(out, "\n  max_2_5: %d\n", best);
        for (int d = 0; d < 4; d++) {
            fprintf(out, "  witness_class%d:", d + 2);
            for (int i = 0; i < bestSz[d]; i++) {
                const auto& x = T.t[POOL[bestCfg[d][i]]];
                fprintf(out, " %d%d%d%d", x[0], x[1], x[2], x[3]);
            }
            fprintf(out, "\n");
        }
        fprintf(out, "\n");
    }

    fprintf(out, "GLOBAL_MAX_2_5 %d\n", globalMax);
    fprintf(out, "BOUND_TWO_FULL_CLASSES %d\n", 12 + globalMax);
    fclose(out);

    printf("\npool sizes: min=%d mean=%.1f max=%d\n", minPool, (double)sumPool / reps.size(), maxPool);
    printf("search nodes total: %lld\n", totalNodes);
    printf("max over all 56 orbits of (classes 2..5) = %d\n", globalMax);
    printf("=> with two full classes the total is at most 6 + 6 + %d = %d\n", globalMax, 12 + globalMax);
    printf("elapsed %.1fs\n", (double)(clock() - t0) / CLOCKS_PER_SEC);

    bool ok = (reps.size() == 56) && (globalMax <= 19);
    printf("CHECK refute: %s (need max <= 19, i.e. no total of 32)\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
