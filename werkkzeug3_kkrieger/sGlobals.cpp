// This file is distributed under a BSD license. See LICENSE.txt for details.
//
// sGlobals.cpp - portable replacement for the global singletons and engine
// support code that historically lived inside the Windows-only backend
// (_start.cpp: Direct3D/DirectSound/DirectInput).  Everything here is
// open-source / POSIX friendly; the actual rendering lives in
// _startgl_sdl.cpp (SDL2 + OpenGL).

#include "_types.hpp"
#include "_start.hpp"
#include "_gui.hpp"
#include "_util.hpp"
#include "material11.hpp"   // sMBF_* material blend flags
#include "_diskitem.hpp"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/****************************************************************************/
/***                                                                      ***/
/***   Global singletons (defined as null until InitX() creates them)     ***/
/***                                                                      ***/
/****************************************************************************/

sBroker_ *sBroker = 0;
sGuiManager *sGui = 0;
sGuiPainter *sPainter = 0;
class sDiskItem *sDiskRoot = 0;
sInt sFatality = 0;

/****************************************************************************/
/***                                                                      ***/
/***   Fatal error handling                                               ***/
/***                                                                      ***/
/****************************************************************************/

void sFatal(const sChar *format, ...)
{
  va_list ap;
  va_start(ap, format);
  fprintf(stderr, "FATAL: ");
  vfprintf(stderr, format, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  sFatality = 1;
}

/****************************************************************************/
/***                                                                      ***/
/***   Portable replacements for MSVC / Win32 CRT helpers                 ***/
/***                                                                      ***/
/****************************************************************************/

extern "C"
{
  // MSVC _ecvt/_fcvt: convert double to digit string.  Portable version
  // using snprintf; sign is returned separately like the MSVC semantics.
  char *_ecvt(double value, int count, int *dec, int *sign)
  {
    static char buf[64];
    int e;
    *sign = (value < 0) ? 1 : 0;
    if(value < 0) value = -value;
    snprintf(buf, sizeof(buf), "%.*e", count > 0 ? count - 1 : 0, value);
    // parse exponent back into dec
    char *p = strchr(buf, 'e');
    *dec = p ? atoi(p + 1) + 1 : 0;
    if(p)
    {
      // strip exponent and decimal point from mantissa
      if(count < 0) count = 15;
      int n = (int)(p - buf);
      char out[64];
      int oi = 0;
      for(int i = 0;i < n && i < 63;i++)
        if(buf[i] != '.') out[oi++] = buf[i];
      out[oi] = 0;
      snprintf(buf, sizeof(buf), "%s", out);
    }
    return buf;
  }

  char *_fcvt(double value, int count, int *dec, int *sign)
  {
    static char buf[512];
    *sign = (value < 0) ? 1 : 0;
    if(value < 0) value = -value;
    snprintf(buf, sizeof(buf), "%.*f", count, value);
    const char *p = strchr(buf, '.');
    if(p) *dec = (int)(p - buf);
    else
    {
      *dec = (int)strlen(buf);
    }
    // remove the decimal point, keep digits only (MSVC behaviour)
    char out[512];
    int oi = 0;
    for(int i = 0;buf[i] && i < 511;i++)
      if(buf[i] != '.') out[oi++] = buf[i];
    out[oi] = 0;
    snprintf(buf, sizeof(buf), "%s", out);
    return buf;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Win32 time shims (used by kdoc.cpp)                                ***/
/***                                                                      ***/
/****************************************************************************/

struct PortFileTime { unsigned long lo, hi; };
struct PortSystemTime
{
  unsigned short wYear, wMonth, wDayOfWeek, wDay, wHour, wMinute, wSecond, wMilliseconds;
};

extern "C"
{
  void GetSystemTime(PortSystemTime *st)
  {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm tmv;
    gmtime_r(&ts.tv_sec, &tmv);
    st->wYear = (unsigned short)(tmv.tm_year + 1900);
    st->wMonth = (unsigned short)(tmv.tm_mon + 1);
    st->wDayOfWeek = (unsigned short)tmv.tm_wday;
    st->wDay = (unsigned short)tmv.tm_mday;
    st->wHour = (unsigned short)tmv.tm_hour;
    st->wMinute = (unsigned short)tmv.tm_min;
    st->wSecond = (unsigned short)tmv.tm_sec;
    st->wMilliseconds = (unsigned short)(ts.tv_nsec / 1000000);
  }

  // FILETIME epoch conversion: 1601-01-01 -> 1970-01-01 offset in 100ns units
  void SystemTimeToFileTime(const PortSystemTime *st, PortFileTime *ft)
  {
    struct tm tmv;
    memset(&tmv, 0, sizeof(tmv));
    tmv.tm_year = st->wYear - 1900;
    tmv.tm_mon = st->wMonth - 1;
    tmv.tm_mday = st->wDay;
    tmv.tm_hour = st->wHour;
    tmv.tm_min = st->wMinute;
    tmv.tm_sec = st->wSecond;
    long long secs = (long long)timegm(&tmv);
    long long hundos = (secs + 11644473600LL) * 10000000LL + st->wMilliseconds * 10000LL;
    ft->lo = (unsigned long)(hundos & 0xffffffff);
    ft->hi = (unsigned long)(hundos >> 32);
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Math helpers exported from _types (were defined in _start.cpp)     ***/
/***                                                                      ***/
/****************************************************************************/

#include <math.h>

sF64 sFPow(sF64 a, sF64 b) { return pow(a, b); }
sF64 sFMod(sF64 a, sF64 b) { return fmod(a, b); }

/****************************************************************************/
/***                                                                      ***/
/***   Intro globals referenced by geneffect.cpp                          ***/
/***                                                                      ***/
/****************************************************************************/

sInt IntroHighTexRes = 0;
sInt IntroStereo3D = 0;

/****************************************************************************/
/***                                                                      ***/
/***   Scene wireframe settings (defined in _start.cpp on Windows)        ***/
/***                                                                      ***/
/****************************************************************************/

sInt SceneWireFlags = 0;
sU32 SceneWireMask = 0;
sInt SceneWireframe = 0;

/****************************************************************************/
/***                                                                      ***/
/***   Engine support classes ported from _start.cpp                      ***/
/***   (texture info init, material environment, simple material)         ***/
/***                                                                      ***/
/****************************************************************************/

void sTexInfo::Init(sBitmap *bm, sInt format, sU32 flags)
{
  XSize = bm ? bm->XSize : 0;
  YSize = bm ? bm->YSize : 0;
  Bitmap = bm;
  Flags = flags;
  Format = format;
}

void sTexInfo::InitRT(sInt xs, sInt ys)
{
  XSize = xs;
  YSize = ys;
  Flags = sTIF_RENDERTARGET;
  Format = 0;
}

void sMaterialEnv::Init()
{
  sSetMem(this, 0, sizeof(*this));

  ModelSpace.Init();
  CameraSpace.Init();
  NearClip = 0.125f;
  FarClip = 4096.0f;
  ZoomX = 1.0f;
  ZoomY = 1.0f;
  CenterX = 0.0f;
  CenterY = 0.0f;
  FogStart = 0;
  FogEnd = FarClip;
  FogColor = 0xff808080;
}

void sMaterialEnv::MakeProjectionMatrix(sMatrix &pmat) const
{
  sF32 q;
  sF32 shiftX = 1.0f / sSystem->ViewportX;
  sF32 shiftY = 1.0f / sSystem->ViewportY;

  switch(Orthogonal)
  {
  case sMEO_PIXELS: // 0 .. screen_max
    pmat.i.Init(2.0f/sSystem->ViewportX, 0, 0, 0);
    pmat.j.Init(0, -2.0f/sSystem->ViewportY, 0, 0);
    pmat.k.Init(0, 0, 1, 0);
    pmat.l.Init(-1 - (2*CenterX+1)*shiftX, 1 + (2*CenterY+1)*shiftY, 0, 1);
    break;

  case sMEO_NORMALISED: // -1 .. 1
    pmat.i.Init(ZoomX, 0, 0, 0);
    pmat.j.Init(0, ZoomY, 0, 0);
    pmat.k.Init(0, 0, 1.0f/FarClip, 0);
    pmat.l.Init(-shiftX, shiftY, 0, 1);
    break;

  case sMEO_PERSPECTIVE:
    q = 1.0f;
    pmat.i.Init(ZoomX, 0, 0, 0);
    pmat.j.Init(0, ZoomY, 0, 0);
    pmat.k.Init(CenterX, CenterY, q, 1);
    pmat.l.Init(-shiftX, shiftY, -q*NearClip, 0);
    break;
  }
}

sMaterial::~sMaterial()
{
}

void sMaterialSetup::Cleanup()
{
  delete[] States;
  delete[] VSCode;
  delete[] PSCode;

  States = 0;
  VSCode = 0;
  PSCode = 0;
}

sMaterialInstance sMaterialInstance::Null = { 0 };

// Simple material: builds a packed render-state stream understood by the SDL
// backend's SetStates() walker (blend/z/cull approximation) plus a passthrough
// vertex-shader placeholder.  The DX bytecode arrays of the original were
// D3D-specific and are dropped here.
sSimpleMaterial::sSimpleMaterial(sInt tex, sU32 flags, sU32 flags2, sU32 color)
{
  sU32 states[64], *st = states;

  sInt blendMode = flags & sMBF_BLENDMASK;
  (void)flags2;

  // state tokens match the values handled by sSystem_::SetState in
  // _startgl_sdl.cpp (0x0b3 alpha-blend enable, 0x0a4 zfunc, ...).
  *st++ = 0x0de; *st++ = (flags & sMBF_ZWRITE) ? 1 : 0;   // zwrite
  *st++ = 0x0a4; *st++ = (flags & sMBF_ZREAD) ? 2 : 7;    // zfunc LESS / ALWAYS
  *st++ = 0x0b3;                                          // alpha blend enable below
  switch(blendMode)
  {
  case sMBF_BLENDADD:
    *st++ = 1; *st++ = 0x0b4; *st++ = 2;                  // src=ONE
    *st++ = 0x0b5; *st++ = 2;                             // dst=ONE
    break;
  case sMBF_BLENDALPHA:
    *st++ = 1; *st++ = 0x0b4; *st++ = 6;                  // src=SRCALPHA
    *st++ = 0x0b5; *st++ = 7;                             // dst=INVSRCALPHA
    break;
  default:
    *st++ = 0;
    break;
  }
  *st++ = 0;                                              // end of stream

  sU32 *heapstates = new sU32[st - states + 1];
  memcpy(heapstates, states, (st - states) * sizeof(sU32));
  heapstates[st - states] = 0;

  Setup = sSystem->MtrlAddSetup(heapstates, 0, 0);
  Tex = tex;
  Color = color;
}

sSimpleMaterial::~sSimpleMaterial()
{
  sSystem->MtrlRemSetup(Setup);
}

void sSimpleMaterial::SetTex(sInt tex)
{
  Tex = tex;
}

void sSimpleMaterial::Set(const sMaterialEnv &env)
{
  sMaterialInstance inst;
  sMatrix mat;

  inst.NumTextures = 1;
  inst.Textures[0] = Tex;
  inst.NumVSConstants = 4;
  inst.VSConstants = &mat.i;
  inst.NumPSConstants = 0;

  mat.Mul4(env.ModelSpace, sSystem->LastViewProject);
  mat.Trans4();

  sSystem->MtrlSetSetup(Setup);
  sSystem->MtrlSetInstance(inst);
}

/****************************************************************************/
/***                                                                      ***/
/***   Host font fallback (no GDI): fixed-cell bitmap glyphs              ***/
/***   Rendered with a tiny built-in 5x7 ASCII rasterizer.                ***/
/***                                                                      ***/
/****************************************************************************/

static const sU8 *HostFontGlyph(sChar c);       // 5x7 rows in port_shims.cpp

sHostFont::sHostFont()
{
  prv = 0;
  Height = 0;
  Advance = 0;
  Baseline = 0;
}

sHostFont::~sHostFont()
{
}

void sHostFont::Init(sChar *name, sInt height, sInt width, sInt style)
{
  (void)name; (void)style;
  Height = height ? height : 16;
  Advance = width ? width : ((Height * 6) / 8);
  Baseline = Height - (Height / 8) - 1;
}

sInt sHostFont::GetWidth(sChar *text, sInt len)
{
  if(len < 0) len = (sInt)strlen(text);
  return len * Advance;
}

static void RasterizeChar(sBitmap *bm, sInt x, sInt y, sChar c, sU32 color)
{
  const sU8 *g = HostFontGlyph(c);
  if(!bm || !g) return;
  sInt scale = bm->YSize > 0 ? 1 : 1;
  (void)scale;
  for(sInt row = 0;row < 7;row++)
  {
    sU8 bits = g[row];
    for(sInt col = 0;col < 5;col++)
    {
      if(bits & (0x10 >> col))
      {
        sInt px = x + col, py = y + row;
        if(px >= 0 && py >= 0 && px < bm->XSize && py < bm->YSize)
          ((sU32 *)bm->Data)[px + py * bm->XSize] = color;
      }
    }
  }
}

void sHostFont::Print(sBitmap *bm, sChar *text, sInt x, sInt y, sU32 color, sInt len)
{
  if(len < 0) len = (sInt)strlen(text);
  for(sInt i = 0;i < len;i++)
  {
    RasterizeChar(bm, x, y, text[i], color);
    x += Advance;
  }
}

sBool sHostFont::Prepare(sBitmap *bm, sChar *letters, sHostFontLetter *met)
{
  (void)bm;
  for(sInt i = 0;letters[i];i++)
  {
    met[i].Location.x1 = 0;
    met[i].Location.y1 = 0;
    met[i].Location.x1 = 5;
    met[i].Location.y1 = 7;
    met[i].Pre = 0;
    met[i].Post = Advance - 5;
    met[i].Top = 0;
  }
  return sTRUE;
}

sBool sHostFont::PrepareCheck(sInt xs, sInt ys, sChar *letters)
{
  (void)xs; (void)ys; (void)letters;
  return sTRUE;
}

void sHostFont::Begin(sBitmap *bm) { (void)bm; }
void sHostFont::Print(sChar *text, sInt x, sInt y, sU32 c0, sInt len) { (void)text; (void)x; (void)y; (void)c0; (void)len; }
void sHostFont::End(sBitmap *bm) { (void)bm; }

/****************************************************************************/
/***                                                                      ***/
/***   Synthesizer fallback (Virtuz2 external synth was proprietary)      ***/
/***   Produces silence so the audio path stays functional.               ***/
/****************************************************************************/

extern "C"
{
  void synthInit(const void *patchmap, int samplerate) { (void)patchmap; (void)samplerate; }
  void synthRender(void *buf, int smp, void *buf2, int add)
  {
    (void)buf2; (void)add;
    memset(buf, 0, (size_t)smp * 2 * sizeof(short));
  }
  void synthProcessMIDI(const void *ptr) { (void)ptr; }
  void synthSetGlobals(const void *ptr) { (void)ptr; }
}
