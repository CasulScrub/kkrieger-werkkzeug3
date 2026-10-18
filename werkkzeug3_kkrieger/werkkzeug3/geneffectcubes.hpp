// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __GENEFFECTCUBES_HPP__
#define __GENEFFECTCUBES_HPP__

#include "_types.hpp"
#include "geneffect.hpp"

/****************************************************************************/

extern sVector __attribute__((aligned(16))) CubeBuffer[];
extern sInt CubeCount;

/****************************************************************************/

KObject *  Init_Effect_Cubes(KOp *op);
void  Exec_Effect_Cubes(KOp *op,KEnvironment *kenv);

KObject *  Init_Effect_BounceWorm(KOp *op);
void  Exec_Effect_BounceWorm(KOp *op,KEnvironment *kenv,sInt,sF32,sF32,sF32,sF32,sInt,sInt);

KObject *  Init_Effect_Wirbeln(KOp *op);
void  Exec_Effect_Wirbeln(KOp *op,KEnvironment *kenv,sF32,sF32,sF32,sF32,sF32,sF32,sInt,sInt, sF32,sF32,sF32,sInt );

KObject *  Init_Effect_XSI2Cube(KOp *op);
void  Exec_Effect_XSI2Cube(KOp *op,KEnvironment *kenv);

KObject *  Init_Effect_CubeMorpher(KOp *op);
void  Exec_Effect_CubeMorpher(KOp *op,KEnvironment *kenv);

/****************************************************************************/

#endif
