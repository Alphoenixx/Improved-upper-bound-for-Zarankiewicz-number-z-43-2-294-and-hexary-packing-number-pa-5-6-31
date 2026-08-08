// tails.hpp -- rainbow tails over {0..5}^4 and their agreement matrix.
//
// A "word" of a PA(N,5,6) is (c, t) with c in {0..5} the class (first
// coordinate) and t in {0..5}^4 the tail.  Two words agree in at most one
// coordinate.  After normalising class 0 to the diagonal {(0,j,j,j,j)}, every
// word of every other class must have a tail with four DISTINCT entries -- a
// rainbow tail.  There are 6*5*4*3 = 360 of them.
//
// ag(t,u) = number of coordinates where tails t and u agree.
//   same class  (equal first coordinate): need ag == 0
//   cross class (distinct first coords) : need ag <= 1
#pragma once
#include <array>
#include <cstdint>
#include <algorithm>

namespace pa {

struct Tails {
    int n = 0;
    std::array<std::array<uint8_t, 4>, 360> t{};
    std::array<int, 1296> idx{};                       // packed code -> tail index (-1 if not rainbow)
    std::array<std::array<uint8_t, 360>, 360> ag{};    // agreement counts

    Tails() {
        idx.fill(-1);
        for (int a = 0; a < 6; a++)
            for (int b = 0; b < 6; b++)
                for (int c = 0; c < 6; c++)
                    for (int d = 0; d < 6; d++) {
                        if (a == b || a == c || a == d || b == c || b == d || c == d) continue;
                        t[n] = {(uint8_t)a, (uint8_t)b, (uint8_t)c, (uint8_t)d};
                        idx[((a * 6 + b) * 6 + c) * 6 + d] = n;
                        n++;
                    }
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++) {
                int s = 0;
                for (int k = 0; k < 4; k++) if (t[i][k] == t[j][k]) s++;
                ag[i][j] = (uint8_t)s;
            }
    }

    inline int code(int a, int b, int c, int d) const { return ((a * 6 + b) * 6 + c) * 6 + d; }
};

} // namespace pa
