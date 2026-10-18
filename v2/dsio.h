
#ifndef _DSIO_H_
#define _DSIO_H_


typedef void ( DSIOCALLBACK)(void *parm, sF32 *buf, sU32 len);

extern "C" {
	extern sU32  dsInit(DSIOCALLBACK *callback, void *parm, void *hWnd);
	extern void  dsClose();
	extern sS32  dsGetCurSmp();
	extern void  dsSetVolume(float vol);
  extern void  dsTick();
  extern void  dsLock();
  extern void  dsUnlock();
}

#endif