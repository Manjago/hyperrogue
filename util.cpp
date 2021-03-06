// Hyperbolic Rogue
// Copyright (C) 2011-2018 Zeno Rogue, see 'hyper.cpp' for details

// basic utility functions

namespace hr {

#if CAP_TIMEOFDAY
#if !CAP_SDL
int lastusec;
int uticks;

int SDL_GetTicks() {
  struct timeval tim;
  gettimeofday(&tim, NULL);
  int newusec = tim.tv_usec;
  uticks += newusec - lastusec;
  if(newusec <= lastusec)
    uticks += 1000000;
  lastusec = newusec;
  return uticks / 1000;
  }

#endif
#endif

long double sqr(long double x) { return x*x; }
string its(int i) { char buf[64]; sprintf(buf, "%d", i); return buf; }
string fts(float x) { char buf[64]; sprintf(buf, "%4.2f", x); return buf; }
string fts3(float x) { char buf[64]; sprintf(buf, "%5.3f", x); return buf; }
string fts4(float x) { char buf[64]; sprintf(buf, "%6.4f", x); return buf; }
string fts6(float x) { char buf[64]; sprintf(buf, "%8.6f", x); return buf; }
string ftsg(float x) { char buf[64]; sprintf(buf, "%4.2g", x); return buf; }

string ftssmart(ld x) {
  if(x == int(x)) return its(int(x));
  if(abs(x) > 1) return fts4(x);
  char buf[64]; sprintf(buf, "%.10e", (float) x); 
  return buf;
  }

/*
string fts_smartdisplay(ld x, int maxdisplay) {
  string rv;
  if(x > 1e9 || x < -1e9) retrun fts(x);
  if(x<0) { rv = "-"; x = -x; }
  int i = int(x);
  rv += its(i);
  x -= i;
  bool nonzero = i;
  if(x == 0) return rv;
  if(x < 1e-9 && nonzero) return rv;
  rv += ".";
  while(maxdisplay > 0) {
    x *= 10;
    rv += '0' + int(x);
    if(int(x)) nonzero = true;
    x -= int(x);
    if(x == 0) return rv;
    if(x < 1e-9 && nonzero) return rv;
    maxdisplay--;
    }
  } */

string cts(char c) { char buf[8]; buf[0] = c; buf[1] = 0; return buf; }
string llts(long long i) {
    // sprintf does not work on Windows IIRC
    if(i < 0) return "-" + llts(-i);
    if(i < 10) return its((int) i);
    return llts(i/10) + its(i%10);
}
string itsh(int i) {static char buf[16]; sprintf(buf, "%03X", i); return buf; }
string itsh2(int i) {static char buf[16]; sprintf(buf, "%02X", i); return buf; }
string itsh8(int i) {static char buf[16]; sprintf(buf, "%08X", i); return buf; }

int gcd(int i, int j) {
  return i ? gcd(j%i, i) : j;
  }

int gmod(int i, int j) {
  i %= j; if(i<0) i += j;
  return i;
  }

int gdiv(int i, int j) {
  return (i - gmod(i, j)) / j;
  }

ld frac(ld x) {
  x -= int(x);
  if(x < 0) x++;
  return x;
  }

// debug utilities

#if CAP_PROFILING

#define FRAMES 64
#define CATS 16

long long proftable[16][FRAMES];
int pframeid;

void profile_frame() { 
  pframeid++; pframeid %=  FRAMES;
  for(int t=0; t<16; t++) proftable[t][pframeid] = 0;
  }

void profile_start(int t) { proftable[t][pframeid] -= getms(); }
void profile_stop(int t) { proftable[t][pframeid] += getms(); }

void profile_info() {
  for(int t=0; t<16; t++) {
    sort(proftable[t], proftable[t]+FRAMES);
    if(proftable[t][FRAMES-1] == 0) continue;
    long long sum = 0;
    for(int f=0; f<FRAMES; f++) sum += proftable[t][f];
    printf("Category %d: avg = %Ld, %Ld..%Ld..%Ld..%Ld..%Ld\n",
      t, sum / FRAMES, proftable[t][0], proftable[t][16], proftable[t][32],
      proftable[t][48], proftable[t][63]);
    }
  }

#else

#define profile_frame()
#define profile_start(t)
#define profile_stop(t)
#define profile_info()
#endif

int whateveri, whateveri2;

purehookset hooks_tests;

string simplify(const string& s) {
  string res;
  for(char c: s) if(isalnum(c)) res += c;
  return res;
  }

bool appears(const string& haystack, const string& needle) {
  return simplify(haystack).find(simplify(needle)) != string::npos;
  }

cld exp_parser::parse(int prio) {
  cld res;
  while(next() == ' ') at++;
  if(eat("sin(")) res = sin(parsepar());
  else if(eat("cos(")) res = cos(parsepar());
  else if(eat("sinh(")) res = sinh(parsepar());
  else if(eat("cosh(")) res = cosh(parsepar());
  else if(eat("asin(")) res = asin(parsepar());
  else if(eat("acos(")) res = acos(parsepar());
  else if(eat("asinh(")) res = asinh(parsepar());
  else if(eat("acosh(")) res = acosh(parsepar());
  else if(eat("exp(")) res = exp(parsepar());
  else if(eat("log(")) res = log(parsepar());
  else if(eat("tan(")) res = tan(parsepar());
  else if(eat("tanh(")) res = tanh(parsepar());
  else if(eat("atan(")) res = atan(parsepar());
  else if(eat("atanh(")) res = atanh(parsepar());
  else if(eat("abs(")) res = abs(parsepar());
  else if(eat("re(")) res = real(parsepar());
  else if(eat("im(")) res = imag(parsepar());
  else if(eat("conj(")) res = std::conj(parsepar());
  else if(eat("floor(")) res = floor(real(parsepar()));
  else if(eat("frac(")) { res = parsepar(); res = res - floor(real(res)); }
  else if(eat("to01(")) { res = parsepar(); return atan(res) / M_PI + 0.5; }
  else if(eat("ifp(")) {
    cld cond = parse(0);
    if(next() != ',') {at = -1; return 0; } at++;
    cld yes = parse(0);
    if(next() != ',') {at = -1; return 0; } at++;
    cld no = parsepar();
    return real(cond) > 0 ? yes : no;
    }  
  else if(eat("rgb(")) {     
    cld val0 = parse(0);
    if(next() != ',') {at = -1; return 0; } at++;
    cld val1 = parse(0);
    if(next() != ',') {at = -1; return 0; } at++;
    cld val2 = parsepar();
    switch(int(real(extra_params["p"]) + .5)) {
      case 1: return val0;
      case 2: return val1;
      case 3: return val2;
      default: return 0;
      }
    }
  else if(eat("let(")) {
    string name;
    while(true) {
      char c = next();
      if((c >= '0' && c <= '9') || (c == '.' && next(1) != '.') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')
        name += c, at++;
      else break;
      }
    if(next() != '=') { at = -1; return 0; }
    at++;
    cld val = parse(0);
    if(next() != ',') { at = -1; return 0; }
    at++;
    dynamicval<cld> d(extra_params[name], val);
    return parsepar();
    }
  else if(next() == '(') at++, res = parsepar(); 
  else {
    string number;
    while(true) {
      char c = next();
      if((c >= '0' && c <= '9') || (c == '.' && next(1) != '.') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')
        number += c, at++;
      else break;
      }
    if(number == "e") res = exp(1);
    else if(number == "i") res = cld(0, 1);
    else if(number == "p" || number == "pi") res = M_PI;
    else if(number == "" && next() == '-') { at++; res = -parse(prio); }
    else if(number == "") at = -1;
    else if(number == "s") res = ticks / 1000.;
    else if(number == "ms") res = ticks;
    else if(number[0] == '0' && number[1] == 'x') res = strtoll(number.c_str()+2, NULL, 16);
    else if(number == "mousex") res = mousex;
    else if(number == "mousey") res = mousey;
    else if(number == "mousez") res = cld(mousex - current_display->xcenter, mousey - current_display->ycenter) / cld(current_display->radius, 0);
    else if(extra_params.count(number)) res = extra_params[number];
    else if(params.count(number)) res = params.at(number);
    else if(number[0] >= 'a' && number[0] <= 'z') at = -1;
    else { std::stringstream ss; res = 0; ss << number; ss >> res; }
    }
  while(true) {
    if(next() == '.' && next(1) == '.' && prio == 0) {
      vector<cld> rest = { res };
      while(next() == '.' && next(1) == '.') {
        at += 2; rest.push_back(parse(10));
        }
      ld v = ticks * (isize(rest)-1.) / anims::period;
      int vf = v;
      v -= vf;
      vf %= (isize(rest)-1);
      res = rest[vf] + (rest[vf+1] - rest[vf]) * v;
      return res;
      }
    else if(next() == '+' && prio <= 10) at++, res = res + parse(20);
    else if(next() == '-' && prio <= 10) at++, res = res - parse(20);
    else if(next() == '*' && prio <= 20) at++, res = res * parse(30);
    else if(next() == '/' && prio <= 20) at++, res = res / parse(30);
    else if(next() == '^') at++, res = pow(res, parse(40));
    else break;
    }
  return res;
  }

ld parseld(const string& s) {
  exp_parser ep;
  ep.s = s;
  return real(ep.parse());
  }

string parser_help() {
  return XLAT("Functions available: %1", 
    "(a)sin(h), (a)cos(h), (a)tan(h), exp, log, abs, re, im, conj, let(t=...,...t...), floor, frac, e, i, pi, s, ms, mousex, mousey, mousez, to01, ifp(a,v,w) [if positive]");
  }

logger hlog;
}
