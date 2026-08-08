// seeds.cpp -- canonical seeds for the "two full rows" normalisation.
//
// A seed is an ordered S-tuple (p_1,...,p_S) of permutations of [6] such that
// {id, p_1, ..., p_S} is an (S+1) x 6 Latin rectangle: for every column y the
// values y, p_1(y), ..., p_S(y) are pairwise distinct.
//
// Symmetries preserving the normalisation L_s(0,y)=y, L_s(1,y)=p_s(y):
//   (a) column permutation g in S_6 with matching symbol relabelling: p -> g p g^-1
//   (b) permuting the S squares
//   (c) swapping grid rows 0 and 1, then relabelling symbols of square s by p_s^-1:
//       p -> p^-1  (this leaves rows 2..5 and the hole pattern intact)
// One representative per orbit of <(a),(b),(c)> is printed.  Orbits are computed
// exactly by marking every image of each representative, so the list is provably
// complete and irredundant.
//
// usage: ./seeds S [noinv]        (S <= 3 uses a dense sieve)
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <algorithm>
#include <functional>
using namespace std;

int main(int argc,char**argv){
    setvbuf(stdout,NULL,_IONBF,0);
    setvbuf(stderr,NULL,_IONBF,0);
	int S = argc>1? atoi(argv[1]) : 3;
	bool useinv = !(argc>2 && string(argv[2])=="noinv");
	if(S<1||S>3){ fprintf(stderr,"this build supports S=1..3\n"); return 1; }
	vector<array<int,6>> P;
	array<int,6> p={0,1,2,3,4,5};
	map<array<int,6>,int> ix;
	do { ix[p]=P.size(); P.push_back(p); } while(next_permutation(p.begin(),p.end()));
	const int M=720;
	vector<int> invIdx(M); vector<int> comp((size_t)M*M);
	for(int a=0;a<M;a++){
		array<int,6> q{}; for(int i=0;i<6;i++) q[P[a][i]]=i; invIdx[a]=ix[q];
		for(int b=0;b<M;b++){ array<int,6> r{}; for(int i=0;i<6;i++) r[i]=P[a][P[b][i]]; comp[(size_t)a*M+b]=ix[r]; }
	}
	vector<int> derang;
	for(int a=0;a<M;a++){ bool d=true; for(int y=0;y<6;y++) if(P[a][y]==y) d=false; if(d) derang.push_back(a); }
	vector<uint64_t> disc((size_t)M*(M/64+1),0);
	auto setd=[&](int a,int b){ disc[(size_t)a*(M/64+1)+(b>>6)] |= 1ULL<<(b&63); };
	auto isdisc=[&](int a,int b){ return (bool)((disc[(size_t)a*(M/64+1)+(b>>6)]>>(b&63))&1ULL); };
	for(int a=0;a<M;a++) for(int b=0;b<M;b++){
		bool d=true; for(int y=0;y<6;y++) if(P[a][y]==P[b][y]){ d=false; break; }
		if(d) setd(a,b);
	}
	fprintf(stderr,"  derangements of [6]                            = %zu   (expected 265)\n", derang.size());
    if(derang.size()!=265){ fprintf(stderr,"CHECK FAILED: derangement count\n"); return 2; }

	size_t KEYS=1; for(int i=0;i<S;i++) KEYS*=720;
	vector<bool> seen(KEYS,false);
	vector<vector<int>> sqperms; { vector<int> q(S); for(int i=0;i<S;i++) q[i]=i; do sqperms.push_back(q); while(next_permutation(q.begin(),q.end())); }
	auto key=[&](const vector<int>&v){ size_t k=0; for(int t=0;t<S;t++) k=k*720+v[t]; return k; };

	vector<vector<int>> reps; vector<int> cur(S); long long ntuples=0;
	std::function<void(int)> rec=[&](int d){
		if(d==S){
			ntuples++;
			if(seen[key(cur)]) return;
			reps.push_back(cur);
			for(int g=0; g<M; g++){
				int gi=invIdx[g];
				vector<int> C(S), Ci(S);
				for(int t=0;t<S;t++){ int x=comp[(size_t)g*M+cur[t]]; C[t]=comp[(size_t)x*M+gi]; Ci[t]=invIdx[C[t]]; }
				for(auto&sp:sqperms){
					vector<int> a(S),b(S);
					for(int t=0;t<S;t++){ a[t]=C[sp[t]]; b[t]=Ci[sp[t]]; }
					seen[key(a)]=true;
					if(useinv) seen[key(b)]=true;
				}
			}
			return;
		}
		for(int a: derang){
			bool ok=true; for(int t=0;t<d;t++) if(!isdisc(cur[t],a)){ ok=false; break; }
			if(ok){ cur[d]=a; rec(d+1); }
		}
	};
	rec(0);
	long long expTuples = -1;
    if(S==1) expTuples=265;
    if(S==2) expTuples=21280;
    if(S==3) expTuples=393120;
    int expSeeds = -1;
    if(S==2 &&  useinv) expSeeds=28;
    if(S==3 &&  useinv) expSeeds=103;
    if(S==3 && !useinv) expSeeds=134;
    fprintf(stderr,"  ordered %d-tuples forming a %dx6 Latin rectangle  = %lld", S, S+1, ntuples);
    if(expTuples>=0) fprintf(stderr,"   (expected %lld)", expTuples);
    fprintf(stderr,"\n");
    fprintf(stderr,"  canonical seeds (inversion symmetry %s)          = %zu", useinv?"ON ":"OFF", reps.size());
    if(expSeeds>=0) fprintf(stderr,"       (expected %d)", expSeeds);
    fprintf(stderr,"\n");
    if(expTuples>=0 && ntuples!=expTuples){ fprintf(stderr,"CHECK FAILED: Latin rectangle count %lld != %lld\n", ntuples, expTuples); return 2; }
    if(expSeeds>=0 && (int)reps.size()!=expSeeds){ fprintf(stderr,"CHECK FAILED: canonical seed count %zu != %d\n", reps.size(), expSeeds); return 2; }
    printf("# S=%d seeds=%zu tuples=%lld inv=%d\n", S, reps.size(), ntuples, (int)useinv);
	for(auto&T:reps){ for(int t=0;t<S;t++){ for(int y=0;y<6;y++) printf("%d",P[T[t]][y]); printf(t+1<S?" ":"\n"); } }
	return 0;
}
