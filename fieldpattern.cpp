// Hyperbolic Rogue -- implementation of the quotient geometries based on fields
// Copyright (C) 2011-2018 Zeno Rogue, see 'hyper.cpp' for details

namespace hr {

namespace fieldpattern {

extern int subpathid;
extern int subpathorder;

bool isprime(int n) {
  for(int k=2; k<n; k++) if(n%k == 0) return false;
  return true;
  }
  
struct matrix {
  int a[3][3];
  int* operator [] (int k) { return a[k]; }
  const int* operator [] (int k) const { return a[k]; }
  };

bool operator == (const matrix& A, const matrix& B) {
  for(int i=0; i<3; i++) for(int j=0; j<3; j++)
    if(A[i][j] != B[i][j]) return false;
  return true;
  }

bool operator != (const matrix& A, const matrix& B) {
  for(int i=0; i<3; i++) for(int j=0; j<3; j++)
    if(A[i][j] != B[i][j]) return true;
  return false;
  }

bool operator < (const matrix& A, const matrix& B) {
  for(int i=0; i<3; i++) for(int j=0; j<3; j++)
    if(A[i][j] != B[i][j]) return A[i][j] < B[i][j];
  return false;
  }

int btspin(int id, int d) {
  return S7*(id/S7) + (id + d) % S7;
  }

struct fpattern {
  
  int Prime, wsquare, Field;
  // we perform our computations in the field Z_Prime[w] where w^2 equals wsquare
  // (or simply Z_Prime for wsquare == 0)

  #define EASY
  // 'easy' assumes that all elements of the field actually used
  // are of form n or mw (not n+mw), and cs and ch are both of form n
  // by experimentation, such cs and ch always exist
  // many computations are much simpler under that assumption
  
  #ifndef EASY
  static int neasy;
  
  int m(int x) { x %= Prime; if(x<0) x+= Prime; return x; }
  #endif
  
  int sub(int a, int b) { 
    #ifdef EASY
    return (a + b * (Prime-1)) % Prime;
    #else
    return m(a%Prime-b%Prime) + Prime * m(a/Prime-b/Prime);
    #endif
    }
  
  int add(int a, int b) { 
    #ifdef EASY
    return (a+b)%Prime;
    #else
    return m(a%Prime+b%Prime) + Prime * m(a/Prime+b/Prime);
    #endif
    }
  
  int mul(int tx, int ty) {
    #ifdef EASY
    return (tx*ty*((tx<0&&ty<0)?wsquare:1)) % Prime;
    #else
    if(tx >= Prime && tx % Prime) neasy++; 
    if(ty >= Prime && ty % Prime) neasy++; 
    int x[2], y[2], z[3];
    for(int i=0; i<3; i++) z[i] = 0;
    for(int i=0; i<2; i++) 
      x[i] = tx%Prime, tx /= Prime;
    for(int i=0; i<2; i++) 
      y[i] = ty%Prime, ty /= Prime;
    for(int i=0; i<2; i++)
    for(int j=0; j<2; j++)
      z[i+j] = (z[i+j] + x[i] * y[j]) % Prime;
    z[0] += z[2] * wsquare;
    
    return m(z[0]) + Prime * m(z[1]);
    #endif
    }
  
  int sqr(int x) { return mul(x,x); }
  
  matrix mmul(const matrix& A, const matrix& B) {
    matrix res;
    for(int i=0; i<3; i++) for(int k=0; k<3; k++) {
  #ifdef EASY
      res[i][k] = 
        (mul(A[i][0], B[0][k]) + mul(A[i][1], B[1][k]) + mul(A[i][2], B[2][k])) % Prime;
  #else
      int t=0;                
      for(int j=0; j<3; j++) t = add(t, mul(A[i][j], B[j][k]));
      res[i][k] = t;
  #endif
      }
    return res;
    }
  
  map<matrix, int> matcode;
  vector<matrix> matrices;
  
  vector<string> qpaths;
  
  vector<matrix> qcoords;
  
  matrix Id, R, P;
  
  matrix strtomatrix(string s) {
    matrix res = Id;
    matrix m = Id;
    for(int i=isize(s)-1; i>=0; i--)
      if(s[i] == 'R') res = mmul(R, res);
      else if (s[i] == 'P') res = mmul(P, res);
      else if (s[i] == 'x') { m[0][0] = -1; res = mmul(m, res); m[0][0] = +1; }
      else if (s[i] == 'y') { m[1][1] = -1; res = mmul(m, res); m[1][1] = +1; }
      else if (s[i] == 'z') { m[2][2] = -1; res = mmul(m, res); m[2][2] = +1; }
    return res;
    }
  
  void addas(const matrix& M, int i) {
    if(!matcode.count(M)) {
      matcode[M] = i;
      for(int j=0; j<isize(qcoords); j++)
        addas(mmul(M, qcoords[j]), i);
      }
    }
  
  void add(const matrix& M) {
    if(!matcode.count(M)) {
      int i = matrices.size();
      matcode[M] = i, matrices.push_back(M);
      for(int j=0; j<isize(qcoords); j++)
        addas(mmul(M, qcoords[j]), i);
      add(mmul(R, M));
      }
    }
  
  #define MXF 1000000
  
  vector<int> connections;
  
  vector<int> inverses;
  vector<int> rrf; // rrf[i] equals gmul(i, S7-1)
  vector<int> rpf; // rpf[i] equals gmul(i, S7)
  
  matrix mpow(matrix M, int N) {
    while((N&1) == 0) N >>= 1, M = mmul(M, M);
    matrix res = M;
    N >>= 1;
    while(N) {      
      M = mmul(M,M); if(N&1) res = mmul(res, M);
      N >>= 1;
      }
    return res;
    }
  
  int gmul(int a, int b) { return matcode[mmul(matrices[a], matrices[b])]; }
  int gpow(int a, int N) { return matcode[mpow(matrices[a], N)]; }

  pair<int,bool> gmul(pair<int, bool> a, int b) { 
    return make_pair(gmul(a.first,b), a.second); 
    }
  
  int order(const matrix& M) {
    int cnt = 1;
    matrix Po = M;
    while(Po != Id) Po = mmul(Po, M), cnt++;
    return cnt;
    }
  
  string decodepath(int i) {
    string s;
    while(i) {
      if(i % S7) i--, s += 'R';
      else i = connections[i], s += 'P';
      }
    return s;
    }
  
  int orderstats();
  
  int cs, sn, ch, sh;
  
  int solve() {
    
    for(int a=0; a<3; a++) for(int b=0; b<3; b++) Id[a][b] = a==b?1:0;
  
    if(!isprime(Prime)) {
      return 1;
      }
  
    for(int pw=1; pw<3; pw++) {
      if(pw>3) break;
      Field = pw==1? Prime : Prime*Prime;
      
      if(pw == 2) {
        for(wsquare=1; wsquare<Prime; wsquare++) {
          int roots = 0;
          for(int a=0; a<Prime; a++) if((a*a)%Prime == wsquare) roots++;
          if(!roots) break;
          }
        } else wsquare = 0;
  
  #ifdef EASY        
      std::vector<int> sqrts(Prime, 0);
      for(int k=1-Prime; k<Prime; k++) sqrts[sqr(k)] = k;
      int fmax = Prime;
  #else
      std::vector<int> sqrts(Field);
      for(int k=0; k<Field; k++) sqrts[sqr(k)] = k;
      int fmax = Field;
  #endif
  
      if(Prime == 13 && wsquare && false) {
        for(int i=0; i<Prime; i++) printf("%3d", sqrts[i]);
        printf("\n");
        }
      
      for(int i=0; i<3; i++) for(int j=0; j<3; j++)
        R[i][j] = P[i][j] = i==j ? 1 : 0;
              
      for(cs=0; cs<fmax; cs++) {
        int sb = sub(1, sqr(cs));
  
        sn = sqrts[sb];
  
        R[0][0] = cs; R[1][1] = cs;
        R[0][1] = sn; R[1][0] = sub(0, sn);
        
        matrix Z = R;
        for(int i=1; i<S7; i++) {
          if(Z == Id) goto nextcs;
          Z = mmul(Z, R);
          }
                
        if(Z != Id) continue;
        if(R[0][0] == 1) continue;
        
        for(ch=2; ch<fmax; ch++) {
          int chx = sub(mul(ch,ch), 1);
          
          sh = sqrts[chx];
          P[0][0] = sub(0, ch);
          P[0][2] = sub(0, sh);
          P[1][1] = Prime-1;
          P[2][0] = sh;
          P[2][2] = ch;
          
          matrix Z1 = mmul(P, R);
          matrix Z = Z1;
          for(int i=1; i<S3; i++) {
            if(Z == Id) goto nextch;
            Z = mmul(Z, Z1);
            }
          if(Z == Id) return 0;
          nextch: ;
          }
        nextcs: ;
        }
      }
  
    return 2;
    }
  
  void build() { 
  
    for(int i=0; i<isize(qpaths); i++) {
      matrix M = strtomatrix(qpaths[i]);
      qcoords.push_back(M);
      printf("Solved %s as matrix of order %d\n", qpaths[i].c_str(), order(M));
      }
    
    matcode.clear(); matrices.clear();
    add(Id);
    if(isize(matrices) != S7) { printf("Error: rotation crash #1 (%d)\n", isize(matrices)); exit(1); }
    
    connections.clear();
    
    for(int i=0; i<(int)matrices.size(); i++) {
    
      matrix M = matrices[i];
      
      matrix PM = mmul(P, M);
      
      add(PM);
  
      if(isize(matrices) % S7) { printf("Error: rotation crash (%d)\n", isize(matrices)); exit(1); }
      
      if(!matcode.count(PM)) { printf("Error: not marked\n"); exit(1); }
  
      connections.push_back(matcode[PM]);
      }
  
    DEBB(DF_FIELD, (debugfile, "Computing inverses...\n"));
    int N = isize(matrices);

    DEBB(DF_FIELD, (debugfile, "Number of heptagons: %d\n", N));
  
    rrf.resize(N); rrf[0] = S7-1;
    for(int i=0; i<N; i++) 
      rrf[btspin(i,1)] = btspin(rrf[i], 1),
      rrf[connections[i]] = connections[rrf[i]];
  
    rpf.resize(N); rpf[0] = S7;
    for(int i=0; i<N; i++) 
      rpf[btspin(i,1)] = btspin(rpf[i], 1),
      rpf[connections[i]] = connections[rpf[i]];
  
    inverses.resize(N);
    inverses[0] = 0;
    for(int i=0; i<N; i++) // inverses[i] = gpow(i, N-1);
      inverses[btspin(i,1)] = rrf[inverses[i]], // btspin(inverses[i],6), 
      inverses[connections[i]] = rpf[inverses[i]];
    
    int errs = 0;
    for(int i=0; i<N; i++) if(gmul(i, inverses[i])) errs++;
    if(errs) printf("errs = %d\n", errs);
      
    if(0) for(int i=0; i<isize(matrices); i++) {
      printf("%5d/%4d", connections[i], inverses[i]);
      if(i%S7 == S7-1) printf("\n");       
      }
    
    DEBB(DF_FIELD, (debugfile, "Built.\n"));
    }
  
  static const int MAXDIST = 120;
  
  vector<char> disthep;
  vector<char> disthex;
  
  vector<char> distwall, distriver, distwall2, distriverleft, distriverright, distflower;
  int distflower0;
  
  vector<eItem> markers;
  
  int getdist(pair<int,bool> a, vector<char>& dists) {
    if(!a.second) return dists[a.first];
    int m = MAXDIST;
    int ma = dists[a.first];
    int mb = dists[connections[btspin(a.first, 3)]];
    int mc = dists[connections[btspin(a.first, 4)]];
    m = min(m, 1 + ma);
    m = min(m, 1 + mb);
    m = min(m, 1 + mc);
    if(m <= 2 && ma+mb+mc <= m*3-2) return m-1; // special case
    m = min(m, 2 + dists[connections[btspin(a.first, 2)]]);
    m = min(m, 2 + dists[connections[btspin(a.first, 5)]]);
    m = min(m, 2 + dists[connections[btspin(connections[btspin(a.first, 3)], 5)]]);
    return m;
    }
  
  int getdist(pair<int,bool> a, pair<int,bool> b) {
    if(a.first == b.first) return a.second == b.second ? 0 : 1;
    if(b.first) a.first = gmul(a.first, inverses[b.first]), b.first = 0;
    return getdist(a, b.second ? disthex : disthep);
    }
   
  int maxdist, otherpole, circrad, wallid, wallorder, riverid;

  int dijkstra(vector<char>& dists, vector<int> indist[MAXDIST]) {
    int N = connections.size();
    dists.resize(N);
    for(int i=0; i<N; i++) dists[i] = MAXDIST-1;
    int maxd = 0;
    for(int i=0; i<MAXDIST; i++) while(!indist[i].empty()) {
      int at = indist[i].back();
      indist[i].pop_back();
      if(dists[at] <= i) continue;
      maxd = i;
      dists[at] = i;
      for(int q=0; q<S7; q++) {
        dists[at] = i;
        if(PURE) // todo-variation: PURE here?
          indist[i+1].push_back(connections[at]);
        else {
          indist[i+2].push_back(connections[at]);
          indist[i+3].push_back(connections[btspin(connections[at], 2)]);
          }
        at = btspin(at, 1);
        }
      }
    return maxd;
    }
  
  void analyze() {

    DEBB(DF_FIELD, (debugfile, "variation = %d\n", int(variation)));
    int N = connections.size();
    
    markers.resize(N);
    
    vector<int> indist[MAXDIST];

    indist[0].push_back(0);
    int md0 = dijkstra(disthep, indist);

    indist[1].push_back(0);
    indist[1].push_back(connections[3]);
    indist[1].push_back(connections[4]);
    indist[2].push_back(connections[btspin(connections[3], 5)]);
    indist[2].push_back(connections[2]);
    indist[2].push_back(connections[5]);
    int md1 = dijkstra(disthex, indist);
    
    maxdist = max(md0, md1);

    otherpole = 0;
    
    for(int i=0; i<N; i+=S7) {
      int mp = 0;
      for(int q=0; q<S7; q++) if(disthep[connections[i+q]] < disthep[i]) mp++;
      if(mp == S7) {
        bool eq = true;
        for(int q=0; q<S7; q++) if(disthep[connections[i+q]] != disthep[connections[i]]) eq = false;
        if(eq) {
          // for(int q=0; q<S7; q++) printf("%3d", disthep[connections[i+q]]);
          // printf(" (%2d) at %d\n", disthep[i], i);
          if(disthep[i] > disthep[otherpole]) otherpole = i;
          // for(int r=0; r<S7; r++) {
          // printf("Matrix: "); for(int a=0; a<3; a++) for(int b=0; b<3; b++)
          //    printf("%4d", matrices[i+r][a][b]); printf("\n");
          //  }
          }
        }
      }
  
    circrad = 99;
  
    for(int i=0; i<N; i++) for(int u=2; u<4; u++) if(disthep[i] < circrad)
      if(disthep[connections[i]] < disthep[i] && disthep[connections[btspin(i,u)]] < disthep[i])
        circrad = disthep[i];
  
    DEBB(DF_FIELD, (debugfile, "maxdist = %d otherpole = %d circrad = %d\n", maxdist, otherpole, circrad));
    
    matrix PRRR = strtomatrix("PRRR");
    matrix PRRPRRRRR = strtomatrix("PRRPRRRRR");
    matrix PRRRP = strtomatrix("PRRRP");
    matrix PRP = strtomatrix("PRP");
    matrix PR = strtomatrix("PR");
    matrix Wall = strtomatrix("RRRPRRRRRPRRRP");

    wallorder = order(Wall);
    wallid = matcode[Wall];
    
    DEBB(DF_FIELD, (debugfile, "wall order = %d\n", wallorder));

#define SETDIST(X, d, it) {int c = matcode[X]; indist[d].push_back(c); if(it == itNone) ; else if(markers[c] && markers[c] != it) markers[c] = itBuggy; else markers[c] = it; }
    
    matrix W = Id;
    for(int i=0; i<wallorder; i++) {
      SETDIST(W, 0, itAmethyst)
      W = mmul(W, Wall);
      }
    W = P;
    for(int i=0; i<wallorder; i++) {
      SETDIST(W, 0, itEmerald)
      W = mmul(W, Wall);
      }
    
    int walldist = dijkstra(distwall, indist);
    DEBB(DF_FIELD, (debugfile, "wall dist = %d\n", walldist));
    
    
    W = strtomatrix("RRRRPR");
    for(int j=0; j<wallorder; j++) {
      W = mmul(W, Wall);
      for(int i=0; i<wallorder; i++) {
        SETDIST(W, 0, itNone)
        SETDIST(mmul(PRRR, W), 1, itNone)
        W = mmul(Wall, W);
        }
      }
    dijkstra(distwall2, indist);
    
    int rpushid = matcode[PRRPRRRRR];
    riverid = 0;
    
    for(int i=0; i<N; i++) {
      int j = i;
      int ipush = gmul(rpushid, i);
      for(int k=0; k<wallorder; k++) {
        if(ipush == j) {
          DEBB(DF_FIELD, (debugfile, "River found at %d:%d\n", i, k));
          riverid = i;
          goto riveridfound;
          }
        j = gmul(j, wallid);
        }
      }
    
    riveridfound: ;

    W = strtomatrix("RRRRPR");
    for(int j=0; j<wallorder; j++) {
      W = mmul(W, Wall);
      for(int i=0; i<wallorder; i++) {
        if(i == 7) SETDIST(W, 0, itCoast)
        if(i == 3) SETDIST(mmul(PRRRP, W), 0, itWhirlpool)
        W = mmul(Wall, W);
        }
      }
    dijkstra(PURE ? distriver : distflower, indist);
    
    W = matrices[riverid];
    for(int i=0; i<wallorder; i++) {
      SETDIST(W, 0, itStatue)
      W = mmul(W, Wall);
      }
    W = mmul(P, W);
    for(int i=0; i<wallorder; i++) {
      SETDIST(W, 0, itSapphire)
      W = mmul(W, Wall);
      }
    W = mmul(PRP, matrices[riverid]);
    for(int i=0; i<wallorder; i++) {
      SETDIST(W, 1, itShard)
      W = mmul(W, Wall);
      }
    W = mmul(PR, matrices[riverid]);
    for(int i=0; i<wallorder; i++) {
      SETDIST(W, 1, itGold)
      W = mmul(W, Wall);
      }
    int riverdist = dijkstra(PURE ? distflower : distriver, indist);
    DEBB(DF_FIELD, (debugfile, "river dist = %d\n", riverdist));
    
    for(int i=0; i<isize(currfp.matrices); i++)
      if(currfp.distflower[i] == 0) {
        distflower0 = currfp.inverses[i]+1;
        break;
        }
    
    if(!PURE) {
      W = matrices[riverid];
      for(int i=0; i<wallorder; i++) {
        SETDIST(W, 0, itStatue)
        W = mmul(W, Wall);
        }
      W = mmul(PR, matrices[riverid]);
      for(int i=0; i<wallorder; i++) {
        SETDIST(W, 0, itGold)
        W = mmul(W, Wall);
        }
      W = mmul(P, matrices[riverid]);
      for(int i=0; i<wallorder; i++) {
        SETDIST(W, 1, itSapphire)
        W = mmul(W, Wall);
        }
      dijkstra(distriverleft, indist);
      W = mmul(PRP, matrices[riverid]);
      for(int i=0; i<wallorder; i++) {
        SETDIST(W, 0, itShard)
        W = mmul(W, Wall);
        }
      W = mmul(P, matrices[riverid]);
      for(int i=0; i<wallorder; i++) {
        SETDIST(W, 0, itSapphire)
        W = mmul(W, Wall);
        }
      W = matrices[riverid];
      for(int i=0; i<wallorder; i++) {
        SETDIST(W, 1, itStatue)
        W = mmul(W, Wall);
        }
      dijkstra(distriverright, indist);
      }
    else {
      W = strtomatrix("RRRRPR");
      for(int j=0; j<wallorder; j++) {
        W = mmul(W, Wall);
        for(int i=0; i<wallorder; i++) {
          if(i == 7) SETDIST(W, 0, itCoast)
          W = mmul(Wall, W);
          }
        }
      dijkstra(distriverleft, indist);
      W = strtomatrix("RRRRPR");
      for(int j=0; j<wallorder; j++) {
        W = mmul(W, Wall);
        for(int i=0; i<wallorder; i++) {
          if(i == 3) SETDIST(mmul(PRRRP, W), 0, itWhirlpool)
          W = mmul(Wall, W);
          }
        }
      dijkstra(distriverright, indist);
      }

    DEBB(DF_FIELD, (debugfile, "wall-river distance = %d\n", distwall[riverid]));
    DEBB(DF_FIELD, (debugfile, "river-wall distance = %d\n", distriver[0]));
    }

  bool easy(int i) {
    return i < Prime || !(i % Prime);
    }
  
  // 11 * 25
  // (1+z+z^3) * (1+z^3+z^4) ==
  // 1+z+z^7 == 1+z+z^2(z^5) == 1+z+z^2(1+z^2) = 1+z+z^2+z^4
  
  void init(int p) {
    Prime = p;
    if(solve()) {
      printf("error: could not solve the fieldpattern\n");
      exit(1);
      }
    build();
    }
    
  fpattern(int p) {
    if(!p) return;
    init(p);
    }
  
  void findsubpath() {
    int N = isize(matrices);
    for(int i=1; i<N; i++)
      if(gpow(i, Prime) == 0) {
        subpathid = i;
        subpathorder = Prime;
        DEBB(DF_FIELD, (debugfile, "Subpath found: %s\n", decodepath(i).c_str()));
        return;
        }
    }
  };

int fpattern::orderstats() {
  int N = isize(matrices);

  #define MAXORD 10000
  int ordcount[MAXORD];
  int ordsample[MAXORD];
  
  for(int i=0; i<MAXORD; i++) ordcount[i] = 0;

  for(int i=0; i<N; i++) {
    int cnt = order(matrices[i]);

    if(cnt < MAXORD) {
      if(!ordcount[cnt]) ordsample[cnt] = i;
      ordcount[cnt]++;
      }
    }
  
  printf("Listing:\n");
  for(int i=0; i<MAXORD; i++) if(ordcount[i])
    printf("Found %4d matrices of order %3d: %s\n", ordcount[i], i, decodepath(ordsample[i]).c_str());
  
  return ordsample[Prime];
  }


fpattern fp43(43);

void info() {
  fpattern fp(0);
  int cases = 0, hard = 0;
  for(int p=0; p<500; p++) {
    fp.Prime = p;
    if(fp.solve() == 0) {
      printf("%4d: wsquare=%d cs=%d sn=%d ch=%d sh=%d\n",
        p, fp.wsquare, fp.cs, fp.sn, fp.ch, fp.sh);
      cases++;
      if(!fp.easy(fp.cs) || !fp.easy(fp.sn) || !fp.easy(fp.ch) || !fp.easy(fp.sn))
        hard++;
      #ifndef EASY
      neasy = 0; 
      #endif
      fp.build();
      #ifndef EASY
      printf("Not easy: %d\n", neasy);
      #endif
      int N = isize(fp.matrices);
      int left = N / fp.Prime;
      printf("Prime decomposition: %d = %d", N, fp.Prime);
      for(int p=2; p<=left; p++) while(left%p == 0) printf("*%d", p), left /= p;
      printf("\n");
      printf("Order of RRP is: %d\n", fp.order(fp.strtomatrix("RRP")));
      printf("Order of RRRP is: %d\n", fp.order(fp.strtomatrix("RRRP")));
      printf("Order of RRRPRRRRRPRRRP is: %d\n", fp.order(fp.strtomatrix("RRRPRRRRRPRRRP")));
      }                                  
    }
  printf("cases found = %d (%d hard)\n", cases, hard);
  }

fpattern current_quotient_field(0), fp_invalid(0);
bool quotient_field_changed;

fpattern& getcurrfp() {
  if(geometry == gFieldQuotient && quotient_field_changed)
    return current_quotient_field;  
  if(S7 == 8 && S3 == 3) {
    static fpattern fp(17);
    return fp;
    }
  if(S7 == 5 && S3 == 4) {
    static fpattern fp(11);
    return fp;
    }
  if(S7 == 6 && S3 == 4) {
    static fpattern fp(13);
    return fp;
    }
  if(S7 == 7 && S3 == 4) {
    static fpattern fp(13);
    return fp;
    }
  if(sphere || euclid) return fp_invalid;
  if(S7 == 7 && S3 == 3)
    return fp43;
  return fp_invalid;
  }

// extra information for field quotient extra configuration

vector<fgeomextra> fgeomextras = {
  fgeomextra(gNormal, 3),
  fgeomextra(gOctagon, 1),
  fgeomextra(g45, 0),
  fgeomextra(g46, 3),
  fgeomextra(g47, 0),
/*  fgeomextra(gSphere, 0),
  fgeomextra(gSmallSphere, 0), -> does not find the prime
  fgeomextra(gEuclid, 0),
  fgeomextra(gEuclidSquare, 0),
  fgeomextra(gTinySphere, 0) */
  };

int current_extra = 0;

void nextPrime(fgeomextra& ex) {
  dynamicval<eGeometry> g(geometry, ex.base);
  int nextprime;
  if(isize(ex.primes))
    nextprime = ex.primes.back().p + 1;
  else
    nextprime = 2;
  while(true) {
    fieldpattern::fpattern fp(0);
    fp.Prime = nextprime;
    if(fp.solve() == 0) {
      fp.build();
      int cells = fp.matrices.size() / S7;
      ex.primes.emplace_back(primeinfo{nextprime, cells, (bool) fp.wsquare});
      break;
      }
    nextprime++;
    }
  }

void nextPrimes(fgeomextra& ex) {
  while(isize(ex.primes) < 4) 
    nextPrime(ex);
  }

void enableFieldChange() {
  fgeomextra& gxcur = fgeomextras[current_extra];
  fieldpattern::quotient_field_changed = true;
  nextPrimes(gxcur);
  dynamicval<eGeometry> g(geometry, gFieldQuotient);
  ginf[geometry].sides = ginf[gxcur.base].sides;
  ginf[geometry].vertex = ginf[gxcur.base].vertex;
  ginf[geometry].distlimit = ginf[gxcur.base].distlimit;
  ginf[geometry].tiling_name = ginf[gxcur.base].tiling_name;
  fieldpattern::current_quotient_field.init(gxcur.primes[gxcur.current_prime_id].p);
  }

}

#define currfp fieldpattern::getcurrfp()

int currfp_gmul(int a, int b) { return currfp.gmul(a,b); }
int currfp_inverses(int i) { return currfp.inverses[i]; }
int currfp_distwall(int i) { return currfp.distwall[i]; }

}
