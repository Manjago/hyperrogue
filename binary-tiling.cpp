
namespace hr {

namespace binary {

  enum bindir {
    bd_right = 0,
    bd_up_right = 1,
    bd_up = 2,
    bd_up_left = 3,
    bd_left = 4,
    bd_down = 5, /* for cells of degree 6 */
    bd_down_left = 5, /* for cells of degree 7 */
    bd_down_right = 6 /* for cells of degree 7 */
    };
  
  int type_of(heptagon *h) {
    return h->c7->type;
    }
  
  // 0 - central, -1 - left, +1 - right
  int mapside(heptagon *h) {
    return h->zebraval;
    }
  
  #if DEBUG_BINARY_TILING
  map<heptagon*, long long> xcode;
  map<long long, heptagon*> rxcode;

  long long expected_xcode(heptagon *h, int d) {
    auto r =xcode[h];
    if(d == 0) return r + 1;
    if(d == 1) return 2*r + 1;
    if(d == 2) return 2*r;
    if(d == 3) return 2*r - 1;
    if(d == 4) return r-1;
    if(d == 5 && type_of(h) == 6) return r / 2;
    if(d == 5 && type_of(h) == 7) return (r-1) / 2;
    if(d == 6 && type_of(h) == 7) return (r+1) / 2;
    breakhere();
    }
  #endif
  
  void breakhere() {
    exit(1);
    }
  
  heptagon *path(heptagon *h, int d, int d1, std::initializer_list<int> p) {
    static int rec = 0;
    rec++; if(rec>100) exit(1);
    // printf("{generating path from %p (%d/%d) dir %d:", h, type_of(h), mapside(h), d);
    heptagon *h1 = h;
    for(int d0: p) {
      // printf(" [%d]", d0);
      h1 = hr::createStep(h1, d0);
      // printf(" %p", h1);
      }

    #if DEBUG_BINARY_TILING    
    if(xcode[h1] != expected_xcode(h, d)) {
      printf("expected_xcode mismatch\n");
      breakhere();
      }
    #endif
    // printf("}\n");
    if(h->move(d) && h->move(d) != h1) {
      printf("already connected to something else (1)\n");
      breakhere();
      }
    if(h1->move(d1) && h1->move(d1) != h) {
      printf("already connected to something else (2)\n");
      breakhere();
      }
    h->c.connect(d, h1, d1, false);
    rec--;
    return h1;
    }
  
  heptagon *build(heptagon *parent, int d, int d1, int t, int side, int delta) {
    auto h = buildHeptagon1(tailored_alloc<heptagon> (t), parent, d, hsOrigin, d1);
    h->distance = parent->distance + delta;
    h->c7 = newCell(t, h);
    h->cdata = NULL;
    h->zebraval = side;
    #if DEBUG_BINARY_TILING
    xcode[h] = expected_xcode(parent, d);
    if(rxcode.count(xcode[h])) {
      printf("xcode clash\n");
      breakhere();
      }
    rxcode[xcode[h]] = h;
    #endif
    return h;
    }
  
  heptagon *createStep(heptagon *parent, int d) {
    auto h = parent;
    switch(d) {
      case bd_right: {
        if(mapside(h) > 0 && type_of(h) == 7) 
          return path(h, d, bd_left, {bd_left, bd_down, bd_right, bd_up});
        else if(mapside(h) >= 0) 
          return build(parent, bd_right, bd_left, type_of(parent) ^ 1, 1, 0);
        else if(type_of(h) == 6)
          return path(h, d, bd_left, {bd_down, bd_right, bd_up, bd_left});
        else
          return path(h, d, bd_left, {bd_down_right, bd_up});
        }
      case bd_left: {
        if(mapside(h) < 0 && type_of(h) == 7) 
          return path(h, d, bd_right, {bd_right, bd_down, bd_left, bd_up});
        else if(mapside(h) <= 0) 
          return build(parent, bd_left, bd_right, type_of(parent) ^ 1, -1, 0);
        else if(type_of(h) == 6)
          return path(h, d, bd_right, {bd_down, bd_left, bd_up, bd_right});
        else
          return path(h, d, bd_right, {bd_down_left, bd_up});
        }
      case bd_up_right: {
        return path(h, d, bd_down_left, {bd_up, bd_right});
        }
      case bd_up_left: {
        return path(h, d, bd_down_right, {bd_up, bd_left});
        }
      case bd_up: 
        return build(parent, bd_up, bd_down, 6, mapside(parent), 1);
      default:
        /* bd_down */
        if(type_of(h) == 6) {
          if(mapside(h) == 0)
            return build(parent, bd_down, bd_up, 6, 0, -1);
          else if(mapside(h) == 1)
            return path(h, d, bd_up, {bd_left, bd_left, bd_down, bd_right});
          else if(mapside(h) == -1)
            return path(h, d, bd_up, {bd_right, bd_right, bd_down, bd_left});
          }
        /* bd_down_left */
        else if(d == bd_down_left) {
          return path(h, d, bd_up_right, {bd_left, bd_down});
          }
        else if(d == bd_down_right) {
          return path(h, d, bd_up_left, {bd_right, bd_down});
          }
        }
    printf("error: case not handled in binary tiling\n");
    breakhere();
    return NULL;
    }
  
  transmatrix parabolic(ld u) {
    return parabolic1(u * vid.binary_width / log(2) / 2);
    }

  void draw() {
    dq::visited.clear();
    dq::enqueue(viewctr.at, cview());
    
    while(!dq::drawqueue.empty()) {
      auto& p = dq::drawqueue.front();
      heptagon *h = get<0>(p);
      transmatrix V = get<1>(p);
      dynamicval<ld> b(band_shift, get<2>(p));
      bandfixer bf(V);
      dq::drawqueue.pop();
      
      cell *c = h->c7;
      if(!do_draw(c, V)) continue;
      drawcell(c, V, 0, false);

      dq::enqueue(h->move(bd_up), V * xpush(-log(2)));
      dq::enqueue(h->move(bd_right), V * parabolic(1));
      dq::enqueue(h->move(bd_left), V * parabolic(-1));
      if(c->type == 6)
        dq::enqueue(h->move(bd_down), V * xpush(log(2)));
    // down_left
      if(c->type == 7) {
        dq::enqueue(h->move(bd_down_left), V * parabolic(-1) * xpush(log(2)));
        dq::enqueue(h->move(bd_down_right), V * parabolic(1) * xpush(log(2)));
        }
      }
    }  
  
  transmatrix relative_matrix(heptagon *h2, heptagon *h1) {
    if(gmatrix0.count(h2->c7) && gmatrix0.count(h1->c7))
      return inverse(gmatrix0[h1->c7]) * gmatrix0[h2->c7];
    transmatrix gm = Id, where = Id;
    while(h1 != h2) {
      if(h1->distance <= h2->distance) {
        if(type_of(h2) == 6)
          h2 = hr::createStep(h2, bd_down), where = xpush(-log(2)) * where;
        else if(mapside(h2) == 1)
          h2 = hr::createStep(h2, bd_left), where = parabolic(+1) * where;
        else if(mapside(h2) == -1)
          h2 = hr::createStep(h2, bd_right), where = parabolic(-1) * where;
        }
      else {
        if(type_of(h1) == 6)
          h1 = hr::createStep(h1, bd_down), gm = gm * xpush(log(2));
        else if(mapside(h1) == 1)
          h1 = hr::createStep(h1, bd_left), gm = gm * parabolic(-1);
        else if(mapside(h1) == -1)
          h1 = hr::createStep(h1, bd_right), gm = gm * parabolic(+1);
        }
      }
    return gm * where;
    }

#if CAP_COMMANDLINE
auto bt_config = addHook(hooks_args, 0, [] () {
  using namespace arg;
  if(argis("-btwidth")) {
    shift_arg_formula(vid.binary_width, delayed_geo_reset);
    return 0;
    }
  return 1;
  });
#endif

  }
}
