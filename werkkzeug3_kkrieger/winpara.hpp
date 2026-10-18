// This file is distributed under a BSD license. See LICENSE.txt for details.
//
// winpara.hpp — portable stub of the Windows parameter dialog (WinPara).
// The original WinPara class implemented an editor-only GUI dialog based on
// the Windows SDK. On the open-source toolchain we keep only the interface
// that the portable core sources (e.g. werkops.cpp) call; every method is a
// documented no-op so the non-GUI logic still compiles and links.
//
// Signatures were derived from the actual call sites in werkops.cpp:
//   EditOp(op)                        - 1 arg (WerkOp*)
//   EditLink(label, link)             - 2 args
//   EditLinkI(label, link, flag)      - 3 args
//   EditGroup(label)                  - 1 arg
//   EditSetGray(gray)                 - 1 arg
//   EditFloat(label, i, cnt, def, minV, maxV, step)  - 7 args
//   EditInt  (label, i, cnt, def, minV, maxV, step)  - 7 args
//   EditFlags(label, i, cnt, names)   - 4 args
//   EditCycle(label, i, cnt, names)   - 4 args
//   EditMask (label, i, mask, cnt, names) - 5 args
//   EditRGBA (label, i, def)          - 3 args
//   EditSpline(label, i)              - 2 args
//   EditString(label, i) / EditText(label, i) / EditFileName(label, i) - 2 args
//   EditBool(label, i, def)           - 3 args
//   AddBox(title, i, cnt, names)      - 4 args
//   AddSpecial(text, i)               - 2 args
//   DefaultF2(i, cnt, minV, maxV)     - 4 args
//   DefaultF3(i, cnt, a, b, c)        - 5 args
//   EditBlobPipe() / EditBlobSpline() / Space() - 0 args
//   EditFloat/EditInt also return sControl* at some call sites (werkops.cpp
//   uses the returned control to tweak ChangeCmd/DoneCmd), so they return a
//   null pointer in the headless stub.

#pragma once

class WerkOp;
class sControl;   // fully defined in _gui.hpp, which precedes this include

// Parameter-window command IDs.  In the original Windows build these were
// handled by the WinPara GUI message loop; the headless open-source stub does
// not dispatch commands, so plain unique values keep the call sites in
// werkops.cpp compiling unchanged.
#define CMD_PARA_RELOAD     0x4001  // "reload" button in texture/font dialogs
#define CMD_PARA_CHCHX      0x4100  // change-notification base for count fields

class WinPara
{
public:
  // --- layout helpers (no-ops in the headless stub) ---
  void Space()                                        {}
  void AddBox(sInt title,sInt i,sInt cnt,const sChar *names) {(void)title;(void)i;(void)cnt;(void)names;}
  void AddSpecial(const sChar *text,sInt i)           {(void)text;(void)i;}

  // --- parameter editors (no-ops; they used to create GUI controls) ---
  void EditGroup(const sChar *label)                  {(void)label;}
  void EditSetGray(sBool gray)                        {(void)gray;}
  void EditOp(WerkOp *op)                             {(void)op;}
  void EditLink(const sChar *label,sInt link)         {(void)label;(void)link;}
  void EditLinkI(const sChar *label,sInt link,sInt flag) {(void)label;(void)link;(void)flag;}
  sControl *EditFloat(const sChar *label,sInt i,sInt cnt,sF32 def,sF32 minV,sF32 maxV,sF32 step)
    {(void)label;(void)i;(void)cnt;(void)def;(void)minV;(void)maxV;(void)step;return nullptr;}
  sControl *EditInt(const sChar *label,sInt i,sInt cnt,sInt def,sInt minV,sInt maxV,sF32 step)
    {(void)label;(void)i;(void)cnt;(void)def;(void)minV;(void)maxV;(void)step;return nullptr;}
  void EditFlags(const sChar *label,sInt i,sInt cnt,const sChar *names)
    {(void)label;(void)i;(void)cnt;(void)names;}
  void EditCycle(const sChar *label,sInt i,sInt cnt,const sChar *names)
    {(void)label;(void)i;(void)cnt;(void)names;}
  void EditMask(const sChar *label,sInt i,sU32 mask,sInt cnt,const sChar *names)
    {(void)label;(void)i;(void)mask;(void)cnt;(void)names;}
  void EditRGBA(const sChar *label,sInt i,sU32 def)   {(void)label;(void)i;(void)def;}
  void EditSpline(const sChar *label,sInt i)          {(void)label;(void)i;}
  void EditString(const sChar *label,sInt i)          {(void)label;(void)i;}
  void EditText(const sChar *label,sInt i)            {(void)label;(void)i;}
  void EditFileName(const sChar *label,sInt i)        {(void)label;(void)i;}
  void EditBool(const sChar *label,sInt i,sInt def)   {(void)label;(void)i;(void)def;}
  void EditBlobPipe()                                 {}
  void EditBlobSpline()                               {}

  // --- value defaults ---
  // In the original Windows GUI these applied default values to the float
  // editors returned by EditFloat (werkops.cpp passes those sControl* here),
  // so they accept an sControl* and no-op in the headless stub.
  void DefaultF2(sInt i,sControl *c,sF32 minV,sF32 maxV) {(void)i;(void)c;(void)minV;(void)maxV;}
  void DefaultF3(sInt i,sControl *c,sInt a,sInt b,sInt c2){(void)i;(void)c;(void)a;(void)b;(void)c2;}
};
