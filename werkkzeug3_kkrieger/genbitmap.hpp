// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __GENBITMAP__
#define __GENBITMAP__

#include "_types.hpp"
#include "kdoc.hpp"

/****************************************************************************/
/****************************************************************************/

void Fade64(sU64 &r,sU64 &c0,sU64 &c1,sInt fade);

struct BilinearContext
{
  sU64 *Src;                      // image data
  sInt XShift;                    // xsize = 1 << XShift
  sInt XSize1,YSize1;             // xsize-1,ysize-1
  sU32 XAm,YAm;                   // x-andmask, y-andmask
};

void BilinearSetup(BilinearContext *ctx,sU64 *src,sInt w,sInt h,sInt b);
void BilinearFilter(BilinearContext *ctx,sU64 *r,sInt u,sInt v);
//void BilinearFilter(sU64 *r,sU64 *src,sInt w,sInt h,sInt u,sInt v,sInt b);
sU64 GetColor64(sU32 c);

/****************************************************************************/

class GenBitmap : public KObject
{
public:
  GenBitmap();
  ~GenBitmap();
  void Copy(KObject *);
  KObject *Copy();

  void Init(sInt x,sInt y);
  void MakeTexture(sInt format=1);

  sU64 *Data;                     // the bitmap itself
  sInt XSize;                     // xsize
  sInt YSize;                     // ysize
  sInt Size;                      // xsize*ysize, saves some bytes of code for common loops
  sInt Format;                    // format to use when creating texture
  sInt Texture;                   // also available as texture (tm)
  sInt TexMipCount;
  sInt TexMipTresh;
};

/****************************************************************************/
/****************************************************************************/

GenBitmap *  Bitmap_Flat(sInt xs,sInt ys,sU32 color);
GenBitmap *  Bitmap_Format(GenBitmap *bm,sInt format,sInt count,sInt tresh);
GenBitmap *  Bitmap_RenderTarget(sInt xs,sInt ys,sU32 format);

GenBitmap *  Bitmap_Merge(sInt mode,sInt count,GenBitmap *b0,...);
GenBitmap *  Bitmap_Color(GenBitmap *bm,sInt mode,sU32 color);
GenBitmap *  Bitmap_Range(GenBitmap *bm,sInt mode,sU32 color0,sU32 color1);
GenBitmap *  Bitmap_GlowRect(GenBitmap *bm,sF32 cx,sF32 cy,sF32 rx,sF32 ry,sF32 sx,sF32 sy,sU32 color,sF32 alpha,sF32 power,sU32 wrap,sU32 bugfix);
GenBitmap *  Bitmap_Dots(GenBitmap *bm,sU32 color0,sU32 color1,sInt count,sInt seed);
GenBitmap *  Bitmap_Blur(GenBitmap *bm,sInt order,sF32 sx,sF32 sy,sF32 amp);
GenBitmap *  Bitmap_Mask(GenBitmap *bm,GenBitmap *bb,GenBitmap *bc,sInt mask);
GenBitmap *  Bitmap_HSCB(GenBitmap *bm,sF32 h,sF32 s,sF32 c,sF32 b);
GenBitmap *  Bitmap_Rotate(GenBitmap *bm,sF32 angle,sF32 sx,sF32 sy,sF32 tx,sF32 ty,sInt border,sInt newWidth,sInt newHeight);
GenBitmap *  Bitmap_RotateMul(GenBitmap *bm,sF32 angle,sF32 sx,sF32 sy,sF32 tx,sF32 ty,sInt border,sU32 color,sInt mode,sInt count,sU32 fade);
GenBitmap *  Bitmap_Distort(GenBitmap *bm,GenBitmap *bb,sF32 dist,sInt border);
GenBitmap *  Bitmap_Normals(GenBitmap *bm,sF32 dist,sInt mode);
GenBitmap *  Bitmap_Light(GenBitmap *bm,sInt subcode,sF32 px,sF32 py,sF32 pz,sF32 da,sF32 db,
                                   sU32 diff,sU32 ambi,sF32 outer,sF32 falloff,sF32 amp);
GenBitmap *  Bitmap_Bump(GenBitmap *bm,GenBitmap *bb,sInt subcode,sF32 px,sF32 py,sF32 pz,sF32 da,sF32 db,
                                  sU32 diff,sU32 ambi,sF32 outer,sF32 falloff,sF32 amp,
                                  sU32 spec,sF32 spow,sF32 samp);
GenBitmap *  Bitmap_Text(KOp *,KEnvironment *,GenBitmap *bm,sF32 x,sF32 y,sF32 width,sF32 height,sU32 col,sU32 flags,sInt extspace,sF32 intspace,sF32 lineskip,sChar *text,sChar *font);
GenBitmap *  Bitmap_Perlin(sInt xs,sInt ys,sInt freq,sInt oct,sF32 fadeoff,sInt seed,sInt mode,sF32 amp,sF32 gamma,sU32 col0,sU32 col1);
GenBitmap *  Bitmap_Cell(sInt xs,sInt ys,sU32 col0,sU32 col1,sU32 col2,sInt count,sInt seed,sF32 amp,sF32 gamma,sInt mode,sF32 mindist,sInt percent,sF32 aspect);
GenBitmap *  Bitmap_Wavelet(GenBitmap *bm,sInt mode,sInt count);
GenBitmap *  Bitmap_Gradient(sInt xs,sInt ys,sU32 col0,sU32 col1,sF32 pos,sF32 angle,sF32 length,sInt mode);
GenBitmap *  Bitmap_Twirl(GenBitmap *bm,sF32 strength,sF32 gamma,sF32 rx,sF32 ry,sF32 cx,sF32 cy,sInt border);
GenBitmap *  Bitmap_Sharpen(GenBitmap *bm,sInt order,sF32 sx,sF32 sy,sF32 amp);

GenBitmap *  Bitmap_Import(KOp *op,sChar *filename);

GenBitmap *  Bitmap_ColorBalance(GenBitmap *bm,sF323 shadows,sF323 midtones,sF323 highlights);
GenBitmap *  Bitmap_Unwrap(GenBitmap *bm,sInt mode);
GenBitmap *  Bitmap_Bricks(sInt bmxs,sInt bmys,sInt color0,sInt color1,sInt colorf,sF32 fugex,sF32 fugey,sInt tx,sInt ty,sInt seed,sInt heads,sInt flags,sF32 side,sF32 colorbalance);
GenBitmap *  Bitmap_Bulge(GenBitmap *bm,sF32 warp);
GenBitmap *  Bitmap_Render(KOp *op,class GenIPP *ipp,sInt xs,sInt ys);
GenBitmap *  Bitmap_RenderAuto(KOp *op,class GenMinMesh *mesh,sInt xs,sInt ys,sInt flags);
void _stdcall Exec_Bitmap_Render(sInt xs,sInt ys);
void _stdcall Exec_Bitmap_RenderAuto(sInt xs,sInt ys,sInt flags);

extern sInt GenBitmapTextureSizeOffset;         // 0 = normal, -1 = smaller, 1 = large

/****************************************************************************/
/****************************************************************************/

#endif
