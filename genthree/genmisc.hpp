// This file is distributed under a BSD license. See LICENSE.txt for details.
#ifndef __GENMISC_HPP__
#define __GENMISC_HPP__

#include "_start.hpp"

class ScriptRuntime;


/****************************************************************************/
/****************************************************************************/

sInt  Sys_PrintDez(sInt i);
sInt  Sys_PrintHex(sInt i);
sInt  Sys_Print(sChar *str);

sBool Sys_Show(ScriptRuntime *sr);            // obsolete
sObject * Sys_Copy(sObject *obj);
void  Sys_LoadSound(class GenPlayer *player,sChar *name,sInt mode);
void  Sys_PlaySound(class GenPlayer *player,sInt song);
sObject *  Sys_FindLabel(sObject *obj,sInt label);
sInt  Sys_Pow(sInt a,sInt b);

/****************************************************************************/

/*
void  Global_CamFrustrum(sInt near_,sInt far_);
void  Global_CamPos(sInt3 t,sInt3 r);
void  Global_CamSet(void);
void  Global_CamMat(class GenMatrix *m);

void  Global_ObjPos(sInt3 s,sInt3 r,sInt3 t);
void  Global_ObjSet(void);
void  Global_ObjMat(class GenMatrix *m);
*/

/****************************************************************************/

class GenMatrix : public sObject
{
public:
  sU32 GetClass() { return sCID_GENMATRIX; }
#if !sINTRO
  GenMatrix();
  void Copy(sObject *);
#endif

  sF323 s,r,t;
  sInt Mode;

  GenMatrix *Parent;

  void Calc(sMatrix &);
};

GenMatrix *  Matrix_Generate(sF323 s,sF323 r,sF323 t,sInt mode);
GenMatrix *  Matrix_Modify(GenMatrix *,sF323 s,sF323 r,sF323 t,sInt mode);
GenMatrix *  Matrix_Label(GenMatrix *,sInt label);

#define MM_INIT       0
#define MM_ORBIT      1
#define MM_LOCKAT     2
#define MM_MUL        3
#define MM_TRANSPOSE  4

/****************************************************************************/

struct GenSplineKey
{
  sF32 Time;
  sF32 Value[4];
};

class GenSpline : public sObject
{
public:
  GenSpline();
  ~GenSpline();
  sU32 GetClass() { return sCID_GENSPLINE; }
#if !sINTRO
  void Clear();
  void Copy(sObject *);
#endif

  sInt Dimensions;
  sF32 Tension;
  sF32 Continuity;
  sF32 Bias;
  sArray<GenSplineKey> Keys;

  void Sort();
  void Calc(sF32 time,sF32 *);
};

GenSpline *  Spline_New(sInt dimensions,sInt tension,sInt continuity);
GenSpline *  Spline_Sort(GenSpline *spline);
GenSpline *  Spline_AddKey1(GenSpline *spline,sInt time,sInt val);
GenSpline *  Spline_AddKey2(GenSpline *spline,sInt time,sInt2 val);
GenSpline *  Spline_AddKey3(GenSpline *spline,sInt time,sInt3 val);
GenSpline *  Spline_AddKey4(GenSpline *spline,sInt time,sInt4 val);

void  Spline_Calc1(GenSpline *spline,sInt time);
void  Spline_Calc2(GenSpline *spline,sInt time);
void  Spline_Calc3(GenSpline *spline,sInt time);
void  Spline_Calc4(GenSpline *spline,sInt time);

/****************************************************************************/
/****************************************************************************/

#endif
