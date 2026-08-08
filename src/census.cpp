// census.cpp -- Proposition 8.3 of the paper (the parameter census) at E = 295.
//
//   build:  c++ -O2 -std=c++17 -o bin/census src/census.cpp
//   run  :  ./bin/census [-quiet]
//   exit :  0 the computed list matches the list printed in the paper
//           1 bad usage or an internal sanity check failed
//           3 the computed list differs from the paper (prints the difference)
//
// Deterministic, single-threaded, no input files, no randomness.
//
// Independent re-derivation of Proposition 8.3 (parameter census) at E = 295.
//
// Line-side selection (parameter a = n_8, budget b = m_8):
//   multiset of short lines, weights w >= 1 summing to 6+b, at most 43-b of them,
//   each carrying s <= min(a, 7-w) degree-eight points,
//   packing bound   sum C(s,2) <= C(a,2)
//   leave bound     b + sum w^2 <= 78
//   admissible iff  V = sum w*s >= 6a
//   data reported   ( sum w^2 , G = sum s )
// Point side is the transpose: swap a <-> b.
//
// Test at (n8,m8):  (I)  6n8 + m8 + sum w^2 <= 42 + Phi
//                   (II) 6m8 + n8 + sum v^2 <= 42 + G
//
// A priori range: Cauchy-Schwarz on (6+b)^2 <= (43-b)(78-b) gives b <= 24,
// so 0 <= n8, m8 <= 24 is exhaustive.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <vector>
#include <utility>
#include <algorithm>
#include <unordered_map>
using namespace std;
static inline int C2(int x){ return x<2?0:x*(x-1)/2; }
struct St { short cnt, cc, v, g; };
const int AMAX = 24, UMAX = 6+AMAX, X2MAX = 78;

// frontier[a][b] : list of (x2, maxG) achievable & admissible
static vector<pair<int,int>> FR[AMAX+1][AMAX+1];

int main(int argc, char** argv){
    bool quiet = false;
    for(int i=1;i<argc;i++){
        if(!strcmp(argv[i],"-quiet")) quiet = true;
        else { fprintf(stderr,"usage: census [-quiet]\n"); return 1; }
    }
    // sanity: confirm the a-priori bound b <= 24
    for(int b=0;b<=42;b++){
        long long lhs=(long long)(6+b)*(6+b), rhs=(long long)(43-b)*(78-b);
        if(b<=24 && lhs>rhs){ printf("bound logic error at b=%d\n",b); return 1; }
        if(b==25 && lhs<=rhs){ printf("b=25 not excluded!\n"); return 1; }
    }
    printf("a priori: 0 <= n8,m8 <= 24 (Cauchy-Schwarz)\n\n");

    for(int a=0;a<=AMAX;a++){
        int Cmax=C2(a), Vt=6*a;
        vector<pair<int,int>> items;
        for(int w=1;w<=7;w++) for(int s=0;s<=min(a,7-w);s++) items.push_back({w,s});

        vector<vector<vector<St>>> dp(UMAX+1, vector<vector<St>>(X2MAX+1));
        dp[0][0].push_back({0,0,0,0});

        for(int u=0;u<UMAX;u++){
            // Push successors, then dedup + Pareto-prune a level as soon as it
            // is complete.  Every item has w >= 1, so level u+1 can receive no
            // further contributions once level u has been expanded.
            for(int x2=0;x2<=X2MAX;x2++){
                if(dp[u][x2].empty()) continue;
                for(const St& it : dp[u][x2]){
                    for(const auto& pr : items){
                        int w=pr.first, s=pr.second;
                        int u2=u+w; if(u2>UMAX) continue;
                        int nx=x2+w*w; if(nx>X2MAX) continue;
                        int nc=it.cc+C2(s); if(nc>Cmax) continue;
                        int ncnt=it.cnt+1; if(ncnt>43) continue;
                        int nv=min(Vt, it.v + w*s);
                        int ng=it.g+s;
                        dp[u2][nx].push_back({(short)ncnt,(short)nc,(short)nv,(short)ng});
                    }
                }
            }
            // level u+1 is now complete (all items have w >= 1): dedup + Pareto prune
            int L=u+1; if(L>UMAX) break;
            for(int x2=0;x2<=X2MAX;x2++){
                auto& V0=dp[L][x2];
                if(V0.size()<2){ continue; }
                unordered_map<uint32_t,short> m; m.reserve(V0.size()*2);
                for(const St& s : V0){
                    uint32_t k=((uint32_t)s.cnt<<20)|((uint32_t)s.cc<<10)|(uint32_t)s.v;
                    auto it=m.find(k);
                    if(it==m.end()||it->second<s.g) m[k]=s.g;
                }
                vector<St> all; all.reserve(m.size());
                for(auto& kv : m){
                    short cnt=(short)(kv.first>>20), cc=(short)((kv.first>>10)&1023), v=(short)(kv.first&1023);
                    all.push_back({cnt,cc,v,kv.second});
                }
                sort(all.begin(),all.end(),[](const St&x,const St&y){
                    if(x.cnt!=y.cnt) return x.cnt<y.cnt;
                    if(x.cc!=y.cc)   return x.cc<y.cc;
                    if(x.v!=y.v)     return x.v>y.v;
                    return x.g>y.g; });
                vector<St> keep;
                for(const St& s : all){
                    bool dom=false;
                    for(const St& k : keep)
                        if(k.cnt<=s.cnt && k.cc<=s.cc && k.v>=s.v && k.g>=s.g){ dom=true; break; }
                    if(!dom) keep.push_back(s);
                }
                V0.swap(keep);
            }
        }

        for(int b=0;b<=AMAX;b++){
            int U=6+b, x2cap=78-b, cntcap=43-b;
            vector<int> best(X2MAX+1,-1);
            for(int x2=0;x2<=min(X2MAX,x2cap);x2++)
                for(const St& s : dp[U][x2])
                    if(s.cnt<=cntcap && s.v>=Vt && s.cc<=Cmax && s.g>best[x2]) best[x2]=s.g;
            for(int x2=0;x2<=X2MAX;x2++) if(best[x2]>=0) FR[a][b].push_back({x2,best[x2]});
        }
        if(!quiet){ printf("  a = %2d of %d done\n", a, AMAX); fflush(stdout); }
    }

    printf("\n(n8,m8) pairs admitting a joint solution of (I) and (II):\n");
    vector<pair<int,int>> surv;
    for(int n8=0;n8<=AMAX;n8++) for(int m8=0;m8<=AMAX;m8++){
        bool feas=false;
        for(const auto& L : FR[n8][m8]){          // (x2 = sum w^2 , G)
            if(feas) break;
            for(const auto& P : FR[m8][n8]){      // (x2 = sum v^2 , Phi)
                int I  = 6*n8 + m8 + L.first  - (42 + P.second);
                int II = 6*m8 + n8 + P.first  - (42 + L.second);
                if(I<=0 && II<=0){ feas=true; break; }
            }
        }
        if(feas) surv.push_back({n8,m8});
    }
    for(auto& p : surv) printf("  (%d,%d)\n", p.first, p.second);

    vector<pair<int,int>> claimed={{0,0},{0,1},{1,0},{1,1},{1,2},{2,1},{2,2},{3,3}};
    printf("\npaper claims exactly: ");
    for(auto&p:claimed) printf("(%d,%d) ",p.first,p.second);
    printf("\ncomputed count = %zu, claimed count = %zu\n", surv.size(), claimed.size());
    sort(surv.begin(),surv.end()); sort(claimed.begin(),claimed.end());
    printf("MATCH: %s\n", (surv==claimed)?"YES -- Proposition 8.3 is CONFIRMED":"NO -- see the two lists");
    if(surv!=claimed){
        for(auto&p:surv) if(!binary_search(claimed.begin(),claimed.end(),p)) printf("  EXTRA in mine: (%d,%d)\n",p.first,p.second);
        for(auto&p:claimed) if(!binary_search(surv.begin(),surv.end(),p)) printf("  MISSING in mine: (%d,%d)\n",p.first,p.second);
    }
    return (surv==claimed) ? 0 : 3;
}
