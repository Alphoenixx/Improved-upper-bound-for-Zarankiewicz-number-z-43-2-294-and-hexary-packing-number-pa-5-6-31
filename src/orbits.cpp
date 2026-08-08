// orbits.cpp -- independent enumeration of the full classes and their orbits.
//
// A "full class" is a set of 6 rainbow tails that pairwise agree in NO
// coordinate (equivalently: the four columns are each a permutation of
// {0..5}).  Total >= 32 forces two full classes; class 0 is normalised to the
// diagonal and class 1 is a full class.  The residual symmetry group is
//     S6 (simultaneous symbol relabelling, preserves the diagonal)
//   x S4 (permutation of the four tail coordinates),
// of order 720 * 24 = 17280.
//
// This program re-derives, from scratch:
//     360      rainbow tails
//     265      derangements in S6
//     393120   full classes
//     661      orbits under S6 alone
//     56       orbits under S6 x S4
// and writes canonical orbit representatives.  A class is stored as the SORTED
// tuple of its 6 tail indices, packed base-360 into a uint64 key; the orbit
// representative is the MINIMUM key in the orbit.  That canonical form depends
// on no ordering convention of the original program.
#include "tails.hpp"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <array>
#include <algorithm>
#include <ctime>

using namespace std;

static pa::Tails T;
static vector<array<uint8_t, 6>> S6;
static vector<array<uint8_t, 4>> S4;
static vector<uint16_t> ACT;   // ACT[(pi*24 + sg)*360 + t] = image tail index
static vector<uint64_t> KEY;   // sorted keys of every full class

static inline uint64_t enc6(array<uint16_t, 6> c) {
    sort(c.begin(), c.end());
    uint64_t k = 0;
    for (int i = 0; i < 6; i++) k = k * 360u + c[i];
    return k;
}
static inline array<uint16_t, 6> dec6(uint64_t k) {
    array<uint16_t, 6> c{};
    for (int i = 5; i >= 0; i--) { c[i] = (uint16_t)(k % 360u); k /= 360u; }
    return c;
}
static inline long findKey(uint64_t k) {
    auto it = lower_bound(KEY.begin(), KEY.end(), k);
    if (it == KEY.end() || *it != k) return -1;
    return (long)(it - KEY.begin());
}

static void dumpClass(FILE* f, uint64_t key) {
    array<uint16_t, 6> c = dec6(key);
    for (int j = 0; j < 6; j++) {
        const auto& x = T.t[c[j]];
        fprintf(f, "%d%d%d%d%s", x[0], x[1], x[2], x[3], j == 5 ? "" : " ");
    }
    fputc('\n', f);
}

int main() {
    clock_t t0 = clock();

    { array<uint8_t, 6> a{0, 1, 2, 3, 4, 5}; do { S6.push_back(a); } while (next_permutation(a.begin(), a.end())); }
    { array<uint8_t, 4> a{0, 1, 2, 3};       do { S4.push_back(a); } while (next_permutation(a.begin(), a.end())); }

    printf("rainbow tails                 : %d\n", T.n);
    printf("group order |S6| x |S4|       : %zu x %zu = %zu\n",
           S6.size(), S4.size(), S6.size() * S4.size());

    // ---- action table -------------------------------------------------
    ACT.assign(S6.size() * S4.size() * 360, 0);
    for (size_t pi = 0; pi < S6.size(); pi++) {
        const auto& P = S6[pi];
        for (size_t sg = 0; sg < S4.size(); sg++) {
            const auto& Q = S4[sg];
            size_t base = (pi * S4.size() + sg) * 360;
            for (int t = 0; t < T.n; t++) {
                const auto& tt = T.t[t];
                int a = P[tt[Q[0]]], b = P[tt[Q[1]]], c = P[tt[Q[2]]], d = P[tt[Q[3]]];
                int im = T.idx[T.code(a, b, c, d)];
                if (im < 0) { printf("FATAL: group action left the rainbow tails\n"); return 1; }
                ACT[base + t] = (uint16_t)im;
            }
        }
    }

    // ---- derangements and full classes --------------------------------
    vector<int> der;
    for (size_t i = 0; i < S6.size(); i++) {
        bool ok = true;
        for (int j = 0; j < 6; j++) if (S6[i][j] == j) { ok = false; break; }
        if (ok) der.push_back((int)i);
    }
    printf("derangements in S6            : %zu\n", der.size());

    KEY.reserve(400000);
    for (int ip : der) {
        const auto& p = S6[ip];
        for (int iq : der) {
            const auto& q = S6[iq];
            bool ok = true;
            for (int j = 0; j < 6; j++) if (p[j] == q[j]) { ok = false; break; }
            if (!ok) continue;
            for (int ir : der) {
                const auto& r = S6[ir];
                ok = true;
                for (int j = 0; j < 6; j++) if (r[j] == p[j] || r[j] == q[j]) { ok = false; break; }
                if (!ok) continue;
                array<uint16_t, 6> c{};
                for (int j = 0; j < 6; j++) c[j] = (uint16_t)T.idx[T.code(j, p[j], q[j], r[j])];
                KEY.push_back(enc6(c));
            }
        }
    }
    sort(KEY.begin(), KEY.end());
    if (adjacent_find(KEY.begin(), KEY.end()) != KEY.end()) { printf("FATAL: duplicate class keys\n"); return 1; }
    printf("full classes                  : %zu\n", KEY.size());

    // ---- orbit enumeration --------------------------------------------
    auto orbits = [&](bool useS4, vector<uint64_t>* repsOut) -> size_t {
        vector<char> vis(KEY.size(), 0);
        size_t nsg = useS4 ? S4.size() : 1;   // S4[0] is the identity
        size_t count = 0;
        for (size_t i = 0; i < KEY.size(); i++) {
            if (vis[i]) continue;
            count++;
            uint64_t rep = KEY[i];
            array<uint16_t, 6> c = dec6(KEY[i]);
            for (size_t pi = 0; pi < S6.size(); pi++)
                for (size_t sg = 0; sg < nsg; sg++) {
                    size_t base = (pi * S4.size() + sg) * 360;
                    array<uint16_t, 6> d{};
                    for (int j = 0; j < 6; j++) d[j] = ACT[base + c[j]];
                    uint64_t k2 = enc6(d);
                    long j2 = findKey(k2);
                    if (j2 < 0) { printf("FATAL: orbit left the family of full classes\n"); exit(1); }
                    vis[j2] = 1;
                    if (k2 < rep) rep = k2;
                }
            if (repsOut) repsOut->push_back(rep);
        }
        return count;
    };

    vector<uint64_t> reps661, reps56;
    size_t o661 = orbits(false, &reps661);
    size_t o56  = orbits(true,  &reps56);
    printf("orbits under S6               : %zu\n", o661);
    printf("orbits under S6 x S4          : %zu\n", o56);

    // ---- sanity: every representative really is a full class -----------
    for (uint64_t k : reps56) {
        array<uint16_t, 6> c = dec6(k);
        for (int a = 0; a < 6; a++)
            for (int b = a + 1; b < 6; b++)
                if (T.ag[c[a]][c[b]] != 0) { printf("FATAL: representative is not a full class\n"); return 1; }
    }

    sort(reps56.begin(), reps56.end());
    sort(reps661.begin(), reps661.end());

    FILE* f = fopen("data/orbits56.txt", "w");
    if (!f) { printf("FATAL: cannot write data/orbits56.txt (run from the repo root)\n"); return 1; }
    fprintf(f, "# %zu canonical representatives of the full classes under S6 x S4\n", reps56.size());
    fprintf(f, "# one class per line, six rainbow tails, each as four digits\n");
    for (uint64_t k : reps56) dumpClass(f, k);
    fclose(f);

    f = fopen("data/orbits661.txt", "w");
    if (!f) { printf("FATAL: cannot write data/orbits661.txt\n"); return 1; }
    fprintf(f, "# %zu canonical representatives under simultaneous S6 conjugation alone\n", reps661.size());
    for (uint64_t k : reps661) dumpClass(f, k);
    fclose(f);

    printf("wrote data/orbits56.txt and data/orbits661.txt\n");
    printf("elapsed %.1fs\n", (double)(clock() - t0) / CLOCKS_PER_SEC);

    bool ok = (T.n == 360) && (der.size() == 265) && (KEY.size() == 393120) && (o661 == 661) && (o56 == 56);
    printf("CHECK orbits: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
