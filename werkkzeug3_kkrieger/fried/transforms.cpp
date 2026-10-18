// This file is distributed under a BSD license. See LICENSE.txt for details.

// FRIED
// transform innerloops

#include "_types.hpp"
#include "fried_internal.hpp"

namespace FRIED
{
  // new 1d dct (11A 6S)
  // b norm: 1.3260 d norm: 1.5104
  static void ndct4(sInt &ar,sInt &br,sInt &cr,sInt &dr)
  {
    sInt a,b,c,d;

    a = ar;
    b = br;
    c = cr;
    d = dr;

    // stage 1
    a += d;
    c -= b;
    d <<= 1;
    d -= a;

    // stage 2
    b += (c - a) >> 1;
    a += b;

    c -= (d >> 1) - (d >> 3);
    d += (c >> 1) - (c >> 3);

    // store (with reordering!)
    ar = a;
    br = d; // !
    cr = b; // !
    dr = c; // !
  }

  // new 1d idct
  static void indct4(sS16 &ar,sS16 &br,sS16 &cr,sS16 &dr)
  {
    sInt a,b,c,d;

    // load (with reordering)!
    a = ar;
    b = cr; // !
    c = dr; // !
    d = br; // !

    // stage 2
    d -= (c >> 1) - (c >> 3);
    c += (d >> 1) - (d >> 3);

    a -= b;
    b -= (c - a) >> 1;

    // stage 1
    d += a;
    d >>= 1;
    c += b;
    a -= d;

    ar = a;
    br = b;
    cr = c;
    dr = d;
  }

  // 1d wht (lifting-based)
  static void wht4(sInt &ar,sInt &br,sInt &cr,sInt &dr)
  {
    sInt a,b,c,d,t;

    // load
    a = ar;
    b = br;
    c = cr;
    d = dr;

    // computation
    a += d;
    c -= b;
    t = (c - a) >> 1;
    d += t;
    b += t;
    c -= d;
    a += b;

    // store (with reordering)
    ar = a;
    br = d; // !
    cr = b; // !
    dr = c; // !
  }

  // 1d iwht
  static void iwht4(sInt &ar,sInt &br,sInt &cr,sInt &dr)
  {
    sInt a,b,c,d,t;

    // load (with reordering)
    a = ar;
    b = cr; // !
    c = dr; // !
    d = br; // !

    // computation
    a -= b;
    c += d;
    t = (c - a) >> 1;
    b -= t;
    d -= t;
    c += b;
    a -= d;

    // store
    ar = a;
    br = b;
    cr = c;
    dr = d;
  }

  // several dct variants. ndcts generate permuted output,
  // indcts expect permuted input. all permutation handling
  // is done during coefficient reordering.
  // 
  // current cost:
  //   88A 48S
  // 
  // dct 4x4:  72A 24M
  // h.264 IT: 64A 16S

  void ndct42D(sInt *x0,sInt *x1,sInt *x2,sInt *x3)
  {
    // transpose in
    sSwap(x0[1],x1[0]);
    sSwap(x0[2],x2[0]);
    sSwap(x0[3],x3[0]);
    sSwap(x1[2],x2[1]);
    sSwap(x1[3],x3[1]);
    sSwap(x2[3],x3[2]);

    // horizontal
    ndct4(x0[0],x0[1],x0[2],x0[3]);
    ndct4(x1[0],x1[1],x1[2],x1[3]);
    ndct4(x2[0],x2[1],x2[2],x2[3]);
    ndct4(x3[0],x3[1],x3[2],x3[3]);

    // vertical
    ndct4(x0[0],x1[0],x2[0],x3[0]);
    ndct4(x0[1],x1[1],x2[1],x3[1]);
    ndct4(x0[2],x1[2],x2[2],x3[2]);
    ndct4(x0[3],x1[3],x2[3],x3[3]);
  }

  // portable scalar inverse-DCT helper (permuted input, matches indct42D_MB math)
  static void iwht4_s(sS16 &ar,sS16 &br,sS16 &cr,sS16 &dr)
  {
    sInt a,b,c,d,t;

    a = ar; b = cr; c = dr; d = br;   // reorder

    a -= b;
    c += d;
    t = (c - a) >> 1;
    b -= t;
    d -= t;
    c += b;
    a -= d;

    ar = a; br = b; cr = c; dr = d;
  }

  // FIXME: no output shifting as of yet
  // FIXME: no output shifting as of yet
  void indct42D(sS16 *x0,sS16 *x1,sS16 *x2,sS16 *x3)
  {
    // vertical
    iwht4_s(x0[0],x1[0],x2[0],x3[0]);
    iwht4_s(x0[1],x1[1],x2[1],x3[1]);
    iwht4_s(x0[2],x1[2],x2[2],x3[2]);
    iwht4_s(x0[3],x1[3],x2[3],x3[3]);

    // horizontal
    iwht4_s(x0[0],x0[1],x0[2],x0[3]);
    iwht4_s(x1[0],x1[1],x1[2],x1[3]);
    iwht4_s(x2[0],x2[1],x2[2],x2[3]);
    iwht4_s(x3[0],x3[1],x3[2],x3[3]);
  }
  }

  void ndct42D_MB(sInt *x0,sInt *x1,sInt *x2,sInt *x3)
  {
    // horizontal
    wht4(x0[ 0],x0[ 4],x0[ 8],x0[12]);
    wht4(x1[ 0],x1[ 4],x1[ 8],x1[12]);
    wht4(x2[ 0],x2[ 4],x2[ 8],x2[12]);
    wht4(x3[ 0],x3[ 4],x3[ 8],x3[12]);

    // vertical
    wht4(x0[ 0],x1[ 0],x2[ 0],x3[ 0]);
    wht4(x0[ 4],x1[ 4],x2[ 4],x3[ 4]);
    wht4(x0[ 8],x1[ 8],x2[ 8],x3[ 8]);
    wht4(x0[12],x1[12],x2[12],x3[12]);
  }

  // hardcoded now to save on call costs
  void indct42D_MB(sS16 *x0,sS16 *x1,sS16 *x2,sS16 *x3)
  {
    sInt temp[16];
    sInt i,a,b,c,d,t;

    // vertical
    for(i=0;i<4;i++)
    {
      a = x0[i*4];
      b = x2[i*4];
      c = x3[i*4];
      d = x1[i*4];

      a -= b;
      c += d;
      t = (c - a) >> 1;
      b -= t;
      d -= t;
      c += b;
      a -= d;

      temp[ 0+i] = a;
      temp[ 4+i] = b;
      temp[ 8+i] = c;
      temp[12+i] = d;
    }

    // horizontal
    for(i=0;i<4;i++)
    {
      a = temp[i*4+0];
      b = temp[i*4+2];
      c = temp[i*4+3];
      d = temp[i*4+1];

      a -= b;
      c += d;
      t = (c - a) >> 1;
      b -= t;
      d -= t;
      c += b;
      a -= d;

      switch(i)
      {
      case 0: x0[0] = a; x0[4] = b; x0[8] = c; x0[12] = d; break;
      case 1: x1[0] = a; x1[4] = b; x1[8] = c; x1[12] = d; break;
      case 2: x2[0] = a; x2[4] = b; x2[8] = c; x2[12] = d; break;
      case 3: x3[0] = a; x3[4] = b; x3[8] = c; x3[12] = d; break;
      }
    }
  }

  static void rot_pp(sInt &u,sInt &v)
  {
    v -= u;
    u <<= 1;
    u += v >> 1;
    v += u >> 1;
  }

  static void __attribute__((always_inline)) inline irot_pp(sInt &ur,sInt &vr)
  {
    sInt u,v;

    u = ur;
    v = vr;

    v -= u >> 1;
    u -= v >> 1;
    u >>= 1;
    v += u;

    ur = u;
    vr = v;
  }

  // gain: 2 (+1bit)
  static void lbtpre1D(sInt &a,sInt &b,sInt &c,sInt &d)
  {
    // stage 1 butterfly
    d -= a;
    c -= b;
    a += a + d;
    b += b + c;

    // rotation
    rot_pp(c,d);

    // stage 3 butterfly
    a -= d - 1;
    b -= c;
    c += c + b + 1;
    d += d + a;

    a >>= 1;
    b >>= 1;
    c >>= 1;
    d >>= 1;
  }

  static void lbtpost1D(sS16 &ar,sS16 &br,sS16 &cr,sS16 &dr)
  {
    sInt a,b,c,d;

    a = ar;
    b = br;
    c = cr;
    d = dr;

    // stage 1 butterfly
    d -= a;
    c -= b;
    a += a + d;
    b += b + c;

    // inverse rotation
    irot_pp(c,d);

    // stage 3 butterfly
    a -= d;
    b -= c;
    c += c + b;
    d += d + a;

    ar = a << 1;
    br = b << 1;
    cr = c << 1;
    dr = d << 1;
  }

  // several variants of lbt pre/postfilters
  void lbt4pre2x4(sInt *x0,sInt *x1)
  {
    lbtpre1D(x0[0],x0[1],x0[2],x0[3]);
    lbtpre1D(x1[0],x1[1],x1[2],x1[3]);
  }

  void lbt4post2x4(sS16 *x0,sS16 *x1)
  {
    lbtpost1D(x0[0],x0[1],x0[2],x0[3]);
    lbtpost1D(x1[0],x1[1],x1[2],x1[3]);
  }

  void lbt4pre4x2(sInt *x0,sInt *x1,sInt *x2,sInt *x3)
  {
    lbtpre1D(x0[0],x1[0],x2[0],x3[0]);
    lbtpre1D(x0[1],x1[1],x2[1],x3[1]);
  }

  void lbt4post4x2(sS16 *x0,sS16 *x1,sS16 *x2,sS16 *x3)
  {
    lbtpost1D(x0[0],x1[0],x2[0],x3[0]);
    lbtpost1D(x0[1],x1[1],x2[1],x3[1]);
  }

  void lbt4pre4x4(sInt *x0,sInt *x1,sInt *x2,sInt *x3)
  {
    // horizontal
    lbtpre1D(x0[0],x0[1],x0[2],x0[3]);
    lbtpre1D(x1[0],x1[1],x1[2],x1[3]);
    lbtpre1D(x2[0],x2[1],x2[2],x2[3]);
    lbtpre1D(x3[0],x3[1],x3[2],x3[3]);

    // vertical
    lbtpre1D(x0[0],x1[0],x2[0],x3[0]);
    lbtpre1D(x0[1],x1[1],x2[1],x3[1]);
    lbtpre1D(x0[2],x1[2],x2[2],x3[2]);
    lbtpre1D(x0[3],x1[3],x2[3],x3[3]);
  }

  void lbt4post4x4(sS16 *x0,sS16 *x1,sS16 *x2,sS16 *x3)
  {
    // vertical
    lbtpost1D(x0[0],x1[0],x2[0],x3[0]);
    lbtpost1D(x0[1],x1[1],x2[1],x3[1]);
    lbtpost1D(x0[2],x1[2],x2[2],x3[2]);
    lbtpost1D(x0[3],x1[3],x2[3],x3[3]);

    // horizontal
    lbtpost1D(x0[0],x0[1],x0[2],x0[3]);
    lbtpost1D(x1[0],x1[1],x1[2],x1[3]);
    lbtpost1D(x2[0],x2[1],x2[2],x2[3]);
    lbtpost1D(x3[0],x3[1],x3[2],x3[3]);
}
