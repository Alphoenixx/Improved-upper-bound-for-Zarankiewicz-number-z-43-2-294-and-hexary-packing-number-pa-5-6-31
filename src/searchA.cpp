// searchA.cpp -- exhaustive "two full rows" search for S pairwise orthogonal
// partial Latin squares of order 6 with N filled cells.  Equivalently: a partial
// transversal design TD(S+2,6) with N blocks, equivalently a code of length
// S+2 over 6 symbols with N words and minimum distance S+1.  See PROOF.md.
//
// COMPLETENESS ARGUMENT -- this is what makes the run a proof, not a search:
//
//   * two blocks that share coordinate i agree nowhere else, so each of the 6
//     classes of coordinate i has size at most 6;
//   * hence if N > 6 + 5*5 = 31 then EVERY coordinate has at least two classes
//     of size exactly 6;
//   * so WLOG the row coordinate has two full classes; permute them to rows 0,1;
//   * per-square symbol relabelling normalises row 0 to L_s(0,y) = y;
//   * row 1 is then a seed (p_1..p_S) with {id,p_1..p_S} an (S+1)x6 Latin
//     rectangle, canonicalised in seeds.cpp and verified complete by
//     verify/checkseeds.py;
//   * rows 2..5 carry all H = 36-N holes; those rows are freely permutable and
//     the seed group does not move the multiset of their hole masks, so the
//     masks may be sorted by (popcount, mask).
//
// Therefore if every (seed, canonical hole pattern) pair is infeasible, no such
// object exists.
//
// usage: searchA S N seedfile [options]
//   -expect F|I   require the verdict to be FEASIBLE / INFEASIBLE; exit 3 if not
//   -sum FILE     write a machine-readable KEY=VALUE summary to FILE
//   -every SECS   progress interval (default 2)
//   -quiet        suppress progress lines
//   -allpat       drop the hole-pattern sorting break and enumerate all
//                 C(24,H) patterns in rows 2..5 (independent audit of it)
//   -lo i -hi j   restrict to seeds [i,j)
//   -t SECS       wall-clock limit; a timeout is reported, never a verdict
//
// exit codes: 0 completed (and matched -expect if one was given)
//             1 usage or I/O error
//             3 verdict contradicts -expect
//             4 hit the time limit before finishing (verdict withheld)

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <fstream>
#include <sstream>
#include <functional>
#include <chrono>
using namespace std;

#if defined(_MSC_VER)
#include <intrin.h>
static inline int PC(unsigned x){ return (int)__popcnt(x); }
static inline int CTZ(unsigned x){ unsigned long i; _BitScanForward(&i,(unsigned long)x); return (int)i; }
#else
static inline int PC(unsigned x){ return __builtin_popcount(x); }
static inline int CTZ(unsigned x){ return __builtin_ctz(x); }
#endif

static int S=3, NCELL, H;
static bool hole[6][6];
static int cellx[40], celly[40];
static int rowm[8][6], colm[8][6], symc[8][6];
static unsigned long long pairm[8][8];
static int val[8][40], best[8][40];
static long long nodes=0;
static bool found=false, timedout=false;
static double deadline=1e300;
static int remCol[8][6];
static int seedp[8][6];
static bool full2=false;
static bool allpat=false;
static int bestHole[6][6];

// progress state -- display only, never consulted by the search
static int g_seedIdx=0, g_seedLo=0, g_seedHi=1, g_patIdx=0, g_patTot=1;
static double g_t0=0.0, g_nextProg=1e300, g_every=2.0;
static bool g_quiet=false;

static double nowsec(){
    using namespace std::chrono;
    static const steady_clock::time_point base = steady_clock::now();
    return duration_cast<duration<double> >(steady_clock::now()-base).count();
}

static void emitProgress(double t){
    g_nextProg = t + g_every;
    int span = g_seedHi - g_seedLo; if(span < 1) span = 1;
    double frac = ((double)(g_seedIdx - g_seedLo) + (double)g_patIdx/(double)g_patTot) / (double)span;
    if(frac < 0.0) frac = 0.0;
    if(frac > 1.0) frac = 1.0;
    double el = t - g_t0;
    double rate = el > 1e-9 ? (double)nodes/el/1e6 : 0.0;
    if(frac > 1e-6){
        double eta = el*(1.0-frac)/frac;
        fprintf(stderr,"  [%5.1f%%] seed %3d/%-4d pat %5d/%-5d nodes %.3e  %6.1f Mnode/s  t=%7.1fs  eta~%.0fs\n",
                100.0*frac, g_seedIdx+1, g_seedHi, g_patIdx+1, g_patTot, (double)nodes, rate, el, eta);
    } else {
        fprintf(stderr,"  [%5.1f%%] seed %3d/%-4d pat %5d/%-5d nodes %.3e  %6.1f Mnode/s  t=%7.1fs\n",
                100.0*frac, g_seedIdx+1, g_seedHi, g_patIdx+1, g_patTot, (double)nodes, rate, el);
    }
}

static inline bool rowHall(int ci,int x){
    int cols[6],m=0;
    for(int i=ci;i<NCELL&&cellx[i]==x;i++) cols[m++]=celly[i];
    if(!m) return true;
    for(int s=0;s<S;s++){
        int dom[6];
        for(int k=0;k<m;k++){ dom[k]=63&~rowm[s][x]&~colm[s][cols[k]]; if(!dom[k]) return false; }
        for(int sub=1;sub<(1<<m);sub++){
            int u=0,c=0;
            for(int k=0;k<m;k++) if((sub>>k)&1){ u|=dom[k]; c++; }
            if(PC((unsigned)u)<c) return false;
        }
    }
    return true;
}

static inline bool boundary(int r){
    if(r>=6) return true;
    for(int s=0;s<S;s++){
        for(int y=0;y<6;y++) if(remCol[r][y] > 6-PC((unsigned)colm[s][y])) return false;
        int cap=0, canfull=0;
        for(int v=0;v<6;v++){
            int rb=symc[s][v]+(6-r); if(rb>6) rb=6;
            int cb=0;
            for(int y=0;y<6;y++){ if((colm[s][y]>>v)&1) cb++; else if(remCol[r][y]>0) cb++; }
            int c=rb<cb?rb:cb;
            if(c>=6) canfull++;
            cap+=c;
        }
        if(cap<NCELL) return false;
        if(full2 && canfull<2) return false;
    }
    return true;
}

static void dfs(int ci);

static void chooseSquare(int ci,int x,int y,int s,int*v){
    if(found||timedout) return;
    if(s==S){
        for(int t=0;t<S;t++){ rowm[t][x]|=1<<v[t]; colm[t][y]|=1<<v[t]; symc[t][v[t]]++; val[t][ci]=v[t]; }
        for(int t=0;t<S;t++) for(int u=t+1;u<S;u++) pairm[t][u]|=1ULL<<(v[t]*6+v[u]);
        bool ok=rowHall(ci+1,x);
        if(ok && (ci+1==NCELL || cellx[ci+1]!=x)) ok=boundary(x+1);
        if(ok) dfs(ci+1);
        for(int t=0;t<S;t++){ rowm[t][x]&=~(1<<v[t]); colm[t][y]&=~(1<<v[t]); symc[t][v[t]]--; }
        for(int t=0;t<S;t++) for(int u=t+1;u<S;u++) pairm[t][u]&=~(1ULL<<(v[t]*6+v[u]));
        return;
    }
    int dom = 63 & ~rowm[s][x] & ~colm[s][y];
    for(int t=0;t<s && dom;t++) dom &= (int)((~(pairm[t][s]>>(v[t]*6)))&63ULL);
    while(dom){ int b=CTZ((unsigned)dom); dom&=dom-1; v[s]=b; chooseSquare(ci,x,y,s+1,v); if(found||timedout) return; }
}

static void dfs(int ci){
    if(found||timedout) return;
    if(((++nodes)&0xFFFFF)==0){
        double t=nowsec();
        if(t>deadline){ timedout=true; return; }
        if(!g_quiet && t>=g_nextProg) emitProgress(t);
    }
    if(ci==NCELL){
        found=true;
        for(int t=0;t<S;t++) for(int i=0;i<NCELL;i++) best[t][i]=val[t][i];
        for(int x=0;x<6;x++) for(int y=0;y<6;y++) bestHole[x][y]=hole[x][y];
        return;
    }
    int v[8];
    chooseSquare(ci,cellx[ci],celly[ci],0,v);
}

static vector<array<int,4> > patterns;
static void genPatterns(){
    array<int,4> m;
    m[0]=m[1]=m[2]=m[3]=0;
    std::function<void(int,int,int,int)> rec=[&](int x,int left,int prevH,int prevMask){
        int idx=x-2;
        if(x==6){ if(left==0) patterns.push_back(m); return; }
        int rowsAfter=5-x;
        int hmin = allpat?0:prevH;
        for(int h=hmin;h<=6 && h<=left;h++){
            if(left-h > 6*rowsAfter) continue;
            if(!allpat && left-h < h*rowsAfter) break;
            for(int mask=0;mask<64;mask++){
                if(PC((unsigned)mask)!=h) continue;
                if(!allpat && h==prevH && mask<prevMask) continue;
                m[idx]=mask; rec(x+1,left-h,h,mask);
            }
        }
    };
    rec(2,H,0,0);
}

int main(int argc,char**argv){
    setvbuf(stdout,NULL,_IONBF,0);
    setvbuf(stderr,NULL,_IONBF,0);
    if(argc<4){
        fprintf(stderr,"usage: searchA S N seedfile [-expect F|I] [-sum FILE] [-every SECS]\n");
        fprintf(stderr,"                            [-quiet] [-allpat] [-lo i] [-hi j] [-t SECS]\n");
        return 1;
    }
    S=atoi(argv[1]); int N=atoi(argv[2]);
    H=36-N;
    if(S<1||S>6){ fprintf(stderr,"bad S=%d\n",S); return 1; }
    if(N<1||N>36){ fprintf(stderr,"bad N=%d\n",N); return 1; }

    vector<array<array<int,6>,8> > seeds;
    {
        ifstream f(argv[3]);
        if(!f){ fprintf(stderr,"cannot open seed file: %s\n",argv[3]); return 1; }
        string line;
        while(getline(f,line)){
            if(!line.empty() && line[line.size()-1]=='\r') line.erase(line.size()-1);
            if(line.empty()||line[0]=='#') continue;
            istringstream is(line); string tok; array<array<int,6>,8> sd; int c=0;
            memset(&sd,0,sizeof sd);
            while(is>>tok && c<S){
                if((int)tok.size()!=6){ fprintf(stderr,"malformed seed token '%s'\n",tok.c_str()); return 1; }
                for(int y=0;y<6;y++) sd[c][y]=tok[y]-'0';
                c++;
            }
            if(c==S) seeds.push_back(sd);
        }
    }
    if(seeds.empty()){ fprintf(stderr,"no seeds read from %s\n",argv[3]); return 1; }

    int lo=0, hi=(int)seeds.size(); double tl=-1;
    bool expectSet=false, expectFeasible=false;
    const char* sumPath=NULL;
    for(int i=4;i<argc;i++){
        string o=argv[i];
        if(o=="-lo" && i+1<argc) lo=atoi(argv[++i]);
        else if(o=="-hi" && i+1<argc) hi=atoi(argv[++i]);
        else if(o=="-t" && i+1<argc) tl=atof(argv[++i]);
        else if(o=="-every" && i+1<argc) g_every=atof(argv[++i]);
        else if(o=="-sum" && i+1<argc) sumPath=argv[++i];
        else if(o=="-f2") full2=true;
        else if(o=="-allpat") allpat=true;
        else if(o=="-quiet") g_quiet=true;
        else if(o=="-expect" && i+1<argc){
            string e=argv[++i];
            if(e=="F"||e=="f"){ expectSet=true; expectFeasible=true; }
            else if(e=="I"||e=="i"){ expectSet=true; expectFeasible=false; }
            else { fprintf(stderr,"-expect takes F or I, got '%s'\n",e.c_str()); return 1; }
        }
        else { fprintf(stderr,"bad option %s\n",o.c_str()); return 1; }
    }
    if(hi>(int)seeds.size()) hi=(int)seeds.size();
    if(lo<0) lo=0;
    if(lo>=hi){ fprintf(stderr,"empty seed range [%d,%d)\n",lo,hi); return 1; }

    genPatterns();
    if(patterns.empty()){ fprintf(stderr,"no hole patterns generated for H=%d\n",H); return 1; }

    long long subproblems = (long long)(hi-lo) * (long long)patterns.size();
    fprintf(stderr,"  S=%d K=%d N=%d H=%d  seeds=%d [%d,%d)  patterns=%zu  allpat=%d  subproblems=%lld\n",
            S,S+2,N,H,(int)seeds.size(),lo,hi,patterns.size(),(int)allpat,subproblems);

    g_seedLo=lo; g_seedHi=hi; g_patTot=(int)patterns.size();
    g_t0=nowsec();
    g_nextProg = g_t0 + g_every;
    if(tl>0) deadline = g_t0 + tl;

    for(int si=lo; si<hi && !found && !timedout; si++){
        g_seedIdx=si;
        for(int s=0;s<S;s++) for(int y=0;y<6;y++) seedp[s][y]=seeds[si][s][y];
        for(size_t pidx=0; pidx<patterns.size(); pidx++){
            g_patIdx=(int)pidx;
            array<int,4>& pt = patterns[pidx];
            memset(hole,0,sizeof hole);
            for(int x=2;x<6;x++) for(int y=0;y<6;y++) if((pt[x-2]>>y)&1) hole[x][y]=true;
            NCELL=0;
            for(int x=0;x<6;x++) for(int y=0;y<6;y++) if(!hole[x][y]){ cellx[NCELL]=x; celly[NCELL]=y; NCELL++; }
            if(NCELL!=N){ fprintf(stderr,"internal error: NCELL %d != %d\n",NCELL,N); return 2; }
            for(int r=0;r<=6;r++) for(int y=0;y<6;y++){ int c=0; for(int x=r;x<6;x++) if(!hole[x][y]) c++; remCol[r][y]=c; }
            memset(rowm,0,sizeof rowm); memset(colm,0,sizeof colm);
            memset(pairm,0,sizeof pairm); memset(symc,0,sizeof symc);
            for(int y=0;y<6;y++){
                for(int s=0;s<S;s++){ rowm[s][0]|=1<<y; colm[s][y]|=1<<y; symc[s][y]++; val[s][y]=y; }
                for(int s=0;s<S;s++) for(int t=s+1;t<S;t++) pairm[s][t]|=1ULL<<(y*6+y);
            }
            for(int y=0;y<6;y++){
                for(int s=0;s<S;s++){ int v=seedp[s][y]; rowm[s][1]|=1<<v; colm[s][y]|=1<<v; symc[s][v]++; val[s][6+y]=v; }
                for(int s=0;s<S;s++) for(int t=s+1;t<S;t++) pairm[s][t]|=1ULL<<(seedp[s][y]*6+seedp[t][y]);
            }
            if(!boundary(2)) continue;
            dfs(12);
            if(found||timedout) break;
        }
    }

    double el = nowsec() - g_t0;
    const char* verdict = found ? "FEASIBLE" : (timedout ? "TIMEOUT" : "INFEASIBLE (exhaustive)");
    const char* shortv  = found ? "FEASIBLE" : (timedout ? "TIMEOUT" : "INFEASIBLE");

    printf("  S=%d K=%d N=%d seeds[%d,%d) patterns=%zu allpat=%d  %s  nodes=%lld time=%.2fs\n",
           S,S+2,N,lo,hi,patterns.size(),(int)allpat,verdict,nodes,el);

    if(found){
        printf("  HOLES:"); for(int x=0;x<6;x++) for(int y=0;y<6;y++) if(bestHole[x][y]) printf(" (%d,%d)",x,y);
        printf("\n  BLOCKS %d  (row col",NCELL); for(int s=0;s<S;s++) printf(" sq%d",s); printf(")\n");
        for(int i=0;i<NCELL;i++){ printf("    %d %d",cellx[i],celly[i]); for(int s=0;s<S;s++) printf(" %d",best[s][i]); printf("\n"); }
    } else if(!timedout){
        printf("  exhausted %d seeds x %zu hole patterns = %lld subproblems\n",
               hi-lo, patterns.size(), subproblems);
    }

    if(sumPath){
        FILE* fp=fopen(sumPath,"w");
        if(!fp){ fprintf(stderr,"cannot write summary file %s\n",sumPath); return 1; }
        fprintf(fp,"sum_status=%s\n",shortv);
        fprintf(fp,"sum_nodes=%lld\n",nodes);
        fprintf(fp,"sum_time=%.2f\n",el);
        fprintf(fp,"sum_S=%d\n",S);
        fprintf(fp,"sum_K=%d\n",S+2);
        fprintf(fp,"sum_N=%d\n",N);
        fprintf(fp,"sum_seeds=%d\n",hi-lo);
        fprintf(fp,"sum_patterns=%zu\n",patterns.size());
        fprintf(fp,"sum_subproblems=%lld\n",subproblems);
        fclose(fp);
    }

    if(timedout){
        fprintf(stderr,"  TIME LIMIT reached -- no verdict. Re-run without -t.\n");
        return 4;
    }
    if(expectSet && found != expectFeasible){
        printf("  EXPECTATION FAILED: expected %s, got %s\n",
               expectFeasible?"FEASIBLE":"INFEASIBLE", shortv);
        return 3;
    }
    if(expectSet) printf("  expectation met: %s\n", shortv);
    return 0;
}
