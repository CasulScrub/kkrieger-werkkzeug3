// This file is distributed under a BSD license. See LICENSE.txt for details.
//
// _startgl_sdl.cpp - Open-source rendering/system backend for the kkrieger
// engine family (replaces the proprietary Direct3D9/DirectSound/DirectInput
// layer found in _start.cpp, which remains available only under _WIN32).
//
// Backend stack (all open source):
//   * windowing/input : SDL2        (libSDL2, zlib/libpng-style license)
//   * rendering       : OpenGL 1.x+ (Mesa / libGL)
//   * audio           : SDL audio callback + sSoundHandler
//   * filesystem      : POSIX (stdio, dirent, unistd)
//
// Build:  make render   (see Makefile "render" target, links -lSDL2 -lGL)

#ifndef _WIN32                      // this backend exists only for POSIX builds

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL.h>
#include <GL/gl.h>

#include "_types.hpp"
#include "_start.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Globals                                                            ***/
/***                                                                      ***/
/****************************************************************************/

sSystem_ *sSystem = 0;              // engine-wide system singleton
static char WCmdLineBuf[4096] = ""; // joined commandline for GetCmdLine()

// GL function pointers we need beyond GL 1.1 headers (VBO entry points).
// Resolved at runtime through SDL_GL_GetProcAddress - no GLEW dependency.
#define APIENTRY_GL
typedef void (APIENTRY_GL *PFN_glGenBuffers)(GLsizei n, GLuint *buffers);
typedef void (APIENTRY_GL *PFN_glDeleteBuffers)(GLsizei n, const GLuint *buffers);
typedef void (APIENTRY_GL *PFN_glBindBuffer)(GLenum target, GLuint buffer);
typedef void (APIENTRY_GL *PFN_glBufferData)(GLenum target, GLsizeiptr size,
                                             const void *data, GLenum usage);
typedef void (APIENTRY_GL *PFN_glBufferSubData)(GLenum target, GLintptr offset,
                                                GLsizeiptr size, const void *data);
static PFN_glGenBuffers    pGlGenBuffers    = 0;
static PFN_glDeleteBuffers pGlDeleteBuffers = 0;
static PFN_glBindBuffer    pGlBindBuffer    = 0;
static PFN_glBufferData    pGlBufferData    = 0;
static PFN_glBufferSubData pGlBufferSubData = 0;

static void ResolveVboFunctions()
{
  pGlGenBuffers    = (PFN_glGenBuffers)    SDL_GL_GetProcAddress("glGenBuffers");
  pGlDeleteBuffers = (PFN_glDeleteBuffers) SDL_GL_GetProcAddress("glDeleteBuffers");
  pGlBindBuffer    = (PFN_glBindBuffer)    SDL_GL_GetProcAddress("glBindBuffer");
  pGlBufferData    = (PFN_glBufferData)    SDL_GL_GetProcAddress("glBufferData");
  pGlBufferSubData = (PFN_glBufferSubData) SDL_GL_GetProcAddress("glBufferSubData");
}

/****************************************************************************/
/***                                                                      ***/
/***   Configuration                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void sSetConfig(sU32 flags,sInt xs,sInt ys)
{
  if(!sSystem)
    sSystem = new sSystem_;

  memset((char *)sSystem + sizeof(void *),0,sizeof(sSystem_) - sizeof(void *));
  // On Windows the DX port did "+4" due to vtable packing; on POSIX it's 8.
  // We zero everything except the (empty) vtable slot region.

  // The Windows app requested D3D; on this platform we transparently map
  // sSF_DIRECT3D to the open-source OpenGL backend.
  if(flags & sSF_DIRECT3D)
  {
    flags &= ~sSF_DIRECT3D;
    flags |= sSF_OPENGL;
  }

  sSystem->ConfigFlags = flags;
  sSystem->ConfigX = xs ? xs : 640;
  sSystem->ConfigY = ys ? ys : 480;
}

/****************************************************************************/
/***                                                                      ***/
/***   System Init / Exit                                                 ***/
/***                                                                      ***/
/****************************************************************************/

void sSystem_::InitX()
{
  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0)
  {
    fprintf(stderr,"SDL_Init failed: %s\n",SDL_GetError());
    exit(1);
  }

  // request a compat-profile context with depth+stencil like the DX device had
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,8);

  WScreenCount = 1;
  InitScreens();
}

void sSystem_::InitScreens()
{
  Uint32 sdlflags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
  if(ConfigFlags & sSF_FULLSCREEN) sdlflags |= SDL_WINDOW_FULLSCREEN;
  if(ConfigFlags & sSF_MAXIMISE)   sdlflags |= SDL_WINDOW_MAXIMIZED;

  for(sInt i = 0;i < WScreenCount;i++)
  {
    Screen[i].Window = (sDInt)(uintptr_t)SDL_CreateWindow(
        "kkrieger (open-source build)",
        SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
        ConfigX,ConfigY,sdlflags);
    if(!Screen[i].Window)
    {
      fprintf(stderr,"SDL_CreateWindow failed: %s\n",SDL_GetError());
      exit(1);
    }
    if(!GLContext)
      GLContext = (sDInt)(uintptr_t)SDL_GL_CreateContext((SDL_Window*)Screen[i].Window);
    SDL_GL_MakeCurrent((SDL_Window*)Screen[i].Window,(SDL_GLContext)GLContext);

    SDL_GetWindowSize((SDL_Window*)Screen[i].Window,&Screen[i].XSize,&Screen[i].YSize);
    ViewportX = Screen[i].XSize;
    ViewportY = Screen[i].YSize;
  }

  if(!(ConfigFlags & sSF_WAITVSYNC))
    SDL_GL_SetSwapInterval(0);            // tearing-fast, like INTERVAL_IMMEDIATE
  else
    SDL_GL_SetSwapInterval(1);

  ResolveVboFunctions();

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_TEXTURE_2D);

  CpuMask = 0xFFFFFFFF;               // x86-64 userland: assume full SSE2 set
  HardwareShaderLevel = sPS_20;       // fixed-function emulation reports ps2.0-ish
}

void sSystem_::ExitScreens()
{
  for(sInt i = 0;i < WScreenCount;i++)
    if(Screen[i].Window)
    {
      SDL_DestroyWindow((SDL_Window*)(uintptr_t)Screen[i].Window);
      Screen[i].Window = 0;
    }
  if(GLContext)
  {
    SDL_GL_DeleteContext((SDL_GLContext)(uintptr_t)GLContext);
    GLContext = 0;
  }
}

void sSystem_::Exit()
{
  WAborting = 1;
}

/****************************************************************************/
/***                                                                      ***/
/***   Main Loop (replaces Win32 PeekMessage/DispatchMessage pump)        ***/
/***                                                                      ***/
/****************************************************************************/

static sU32 TranslateKey(SDL_Keycode k)
{
  switch(k)
  {
  case SDLK_BACKSPACE: return sKEY_BACKSPACE;
  case SDLK_TAB:       return sKEY_TAB;
  case SDLK_RETURN:    return sKEY_ENTER;
  case SDLK_ESCAPE:    return sKEY_ESCAPE;
  case SDLK_SPACE:     return sKEY_SPACE;
  case SDLK_UP:        return sKEY_UP;
  case SDLK_DOWN:      return sKEY_DOWN;
  case SDLK_LEFT:      return sKEY_LEFT;
  case SDLK_RIGHT:     return sKEY_RIGHT;
  case SDLK_PAGEUP:    return sKEY_PAGEUP;
  case SDLK_PAGEDOWN:  return sKEY_PAGEDOWN;
  case SDLK_HOME:      return sKEY_HOME;
  case SDLK_END:       return sKEY_END;
  case SDLK_INSERT:    return sKEY_INSERT;
  case SDLK_DELETE:    return sKEY_DELETE;
  default:
    if(k >= SDLK_F1 && k <= SDLK_F12)
      return sKEY_F1 + (k - SDLK_F1);
    if(k >= SDLK_a && k <= SDLK_z)
      return k - SDLK_a + 'A';        // ascii letters, uppercase like VK codes
    if(k >= SDLK_0 && k <= SDLK_9)
      return (sU32)k;
    return 0;
  }
}

void sSystem_::RunLoop()
{
  sAppHandler(sAPPCODE_INIT,0);

  while(!WAborting)
  {
    SDL_Event ev;
    while(SDL_PollEvent(&ev))
    {
      switch(ev.type)
      {
      case SDL_QUIT:
        WAborting = 1;
        KeyBufferPush(sKEY_CLOSE);
        break;

      case SDL_KEYDOWN:
      case SDL_KEYUP:
        {
          sU32 key = TranslateKey(ev.key.keysym.sym);
          if(key)
          {
            if(ev.key.state == SDL_PRESSED)
              KeyBufferPush(key);
            sAppHandler(sAPPCODE_KEY,key);
          }
        }
        break;

      case SDL_MOUSEBUTTONDOWN:
        if(ev.button.button == SDL_BUTTON_LEFT)   MouseButtons|=sKEYQ_MOUSEL;
        if(ev.button.button == SDL_BUTTON_RIGHT)  MouseButtons|=sKEYQ_MOUSER;
        if(ev.button.button == SDL_BUTTON_MIDDLE) MouseButtons|=sKEYQ_MOUSEM;
        break;
      case SDL_MOUSEBUTTONUP:
        if(ev.button.button == SDL_BUTTON_LEFT)   MouseButtons&=~sKEYQ_MOUSEL;
        if(ev.button.button == SDL_BUTTON_RIGHT)  MouseButtons&=~sKEYQ_MOUSER;
        if(ev.button.button == SDL_BUTTON_MIDDLE) MouseButtons&=~sKEYQ_MOUSEM;
        break;
      case SDL_MOUSEWHEEL:
        KeyBufferPush(ev.wheel.y > 0 ? sKEY_WHEELUP : sKEY_WHEELDOWN);
        break;

      case SDL_WINDOWEVENT:
        if(ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
        {
          Screen[0].XSize = ViewportX = ev.window.data1;
          Screen[0].YSize = ViewportY = ev.window.data2;
          KeyBufferPush(sKEY_SIZE);
        }
        if(ev.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
          KeyBufferPush(sKEY_APPFOCUS);
        if(ev.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
          KeyBufferPush(sKEY_APPFOCLOST);
        break;
      }
    }

    Tick();                          // 10ms timeslices -> sAPPCODE_TICK
    sAppHandler(sAPPCODE_FRAME,0);   // game logic per frame
    Render();                        // swap buffers etc.
  }

  sAppHandler(sAPPCODE_EXIT,0);
  ExitScreens();
}

void sSystem_::KeyBufferPush(sU32 key)
{
  if(KeyIndex < MAX_KEYBUFFER)
    KeyBuffer[KeyIndex++] = key;
}

void sSystem_::Tick()
{
  sInt now = GetTime();
  while(TickTime + 10 <= now)
  {
    TickTime += 10;
    sAppHandler(sAPPCODE_TICK,0);
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Timing                                                             ***/
/***                                                                      ***/
/****************************************************************************/

sInt sSystem_::GetTime()
{
  return (sInt)(SDL_GetTicks());
}

sU32 sSystem_::PerfTime()
{
  return (sU32)(SDL_GetPerformanceCounter() * 1000000ULL /
                SDL_GetPerformanceFrequency());
}

void sSystem_::PerfKalib() {}

sInt sSystem_::GetTimeOfDay()
{
  time_t t = time(0);
  struct tm lt;
  localtime_r(&t,&lt);
  return lt.tm_hour*3600 + lt.tm_min*60 + lt.tm_sec;
}

/****************************************************************************/
/***                                                                      ***/
/***   Rendering primitives                                               ***/
/***                                                                      ***/
/****************************************************************************/

void sSystem_::Render()
{
  SDL_GL_SwapWindow((SDL_Window*)(uintptr_t)Screen[0].Window);
}

void sSystem_::Clear(sU32 flags,sU32 color)
{
  GLbitfield bits = 0;
  if(flags & sVCF_COLOR)
  {
    bits |= GL_COLOR_BUFFER_BIT;
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    glClearColor(((color>>16)&0xff)/255.0f,
                 ((color>> 8)&0xff)/255.0f,
                 ((color     )&0xff)/255.0f,
                 ((color>>24 )&0xff)/255.0f);
  }
  if(flags & (sVCF_Z|sVCF_STENCIL))
  {
    bits |= GL_DEPTH_BUFFER_BIT;
    glDepthMask(GL_TRUE);
    glClearDepth(1.0);
  }
  if(flags & sVCF_STENCIL)
  {
    bits |= GL_STENCIL_BUFFER_BIT;
    glStencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);
    glClearStencil(0);
  }
  if(bits)
    glClear(bits);
}

void sSystem_::SetViewport(const sViewport &vp)
{
  CurrentViewport = vp;
  sInt yflip = vp.Window.y1;
  glViewport(vp.Window.x0,yflip,
             vp.Window.x1-vp.Window.x0,vp.Window.y1-vp.Window.y0);
  glScissor(vp.Window.x0,yflip,
            vp.Window.x1-vp.Window.x0,vp.Window.y1-vp.Window.y0);
  glEnable(GL_SCISSOR_TEST);
  ViewportX = vp.Window.x1;
  ViewportY = vp.Window.y1;
}

sBool sSystem_::GetScreenInfo(sInt i,sScreenInfo &info)
{
  if(i != 0) return sFALSE;
  info.XSize = Screen[0].XSize;
  info.YSize = Screen[0].YSize;
  info.FullScreen = (ConfigFlags & sSF_FULLSCREEN) ? sTRUE : sFALSE;
  info.LowQuality = sFALSE;
  info.ShaderLevel = HardwareShaderLevel;
  info.PixelRatio = 1.0f;
  return sTRUE;
}

sInt sSystem_::GetScreenCount() { return WScreenCount; }
sBool sSystem_::GetFullscreen() { return (ConfigFlags & sSF_FULLSCREEN) ? sTRUE : sFALSE; }

void sSystem_::SetGamma(sF32 gamma)
{
  // Mesa supports X11 gamma via vidmode extension historically; not portable.
  (void)gamma;
}

/****************************************************************************/
/***                                                                      ***/
/***   Textures                                                           ***/
/***                                                                      ***/
/****************************************************************************/

sInt sSystem_::AddTexture(const sTexInfo &ti)
{
  for(sInt h = 1;h < MAX_TEXTURE;h++)
  {
    if(Textures[h].RefCount) continue;
    GLuint tex = 0;
    glGenTextures(1,&tex);
    glBindTexture(GL_TEXTURE_2D,tex);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

    sInt xs = ti.XSize? ti.XSize : (ti.Bitmap?ti.Bitmap->XSize:0);
    sInt ys = ti.YSize? ti.YSize : (ti.Bitmap?ti.Bitmap->YSize:0);
    if(xs && ys && ti.Bitmap)
    {
      // engine bitmaps are ARGB8888 little-endian -> GL_RGBA works directly
      glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,xs,ys,0,GL_RGBA,
                   GL_UNSIGNED_BYTE,ti.Bitmap->Data);
    }
    else if(xs && ys)
    {
      glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,xs,ys,0,GL_RGBA,
                   GL_UNSIGNED_BYTE,0);   // rendertarget placeholder
    }

    Textures[h].RefCount = 1;
    Textures[h].XSize = xs;
    Textures[h].YSize = ys;
    Textures[h].MipLevels = 1;
    Textures[h].Flags = ti.Flags;
    Textures[h].Format = ti.Format;
#if defined(_WIN32)
    Textures[h].Tex = 0;
#else
    Textures[h].GLTex = tex;
#endif
    return h;
  }
  return sINVALID;
}

sInt sSystem_::AddTexture(sInt xs,sInt ys,sInt format,sU16 *data,sInt mipcount,sInt miptresh)
{
  sTexInfo ti;
  ti.Init(0,format);
  ti.XSize = xs;
  ti.YSize = ys;
  sInt h = AddTexture(ti);
  if(h != sINVALID && data)
    UpdateTexture(h,data,miptresh);
  (void)mipcount;
  return h;
}

void sSystem_::AddRefTexture(sInt tex)
{
  if(sISHANDLE(tex)) Textures[tex].RefCount++;
}

void sSystem_::RemTexture(sInt handle)
{
  if(!sISHANDLE(handle)) return;
  sHardTex &ht = Textures[handle];
  if(--ht.RefCount <= 0)
  {
#if !defined(_WIN32)
    if(ht.GLTex) { glDeleteTextures(1,&ht.GLTex); ht.GLTex = 0; }
#endif
    ht.RefCount = 0;
  }
}

void sSystem_::GetTextureSize(sInt tex,sInt &w,sInt &h)
{
  w = tex>=0 ? Textures[tex].XSize : 0;
  h = tex>=0 ? Textures[tex].YSize : 0;
}

void sSystem_::UpdateTexture(sInt handle,sBitmap *bm)
{
  if(!sISHANDLE(handle)) return;
#if !defined(_WIN32)
  glBindTexture(GL_TEXTURE_2D,Textures[handle].GLTex);
  glTexSubImage2D(GL_TEXTURE_2D,0,0,0,bm->XSize,bm->YSize,GL_RGBA,
                  GL_UNSIGNED_BYTE,bm->Data);
#endif
}

void sSystem_::UpdateTexture(sInt handle,sU16 *data,sInt miptresh)
{
  if(!sISHANDLE(handle)) return;
#if !defined(_WIN32)
  glBindTexture(GL_TEXTURE_2D,Textures[handle].GLTex);
  glTexSubImage2D(GL_TEXTURE_2D,0,0,0,Textures[handle].XSize,
                  Textures[handle].YSize,GL_RGBA,GL_UNSIGNED_SHORT_4_4_4_4,data);
#endif
  (void)miptresh;
}

void sSystem_::ReadTexture(sInt handle,sU16 *data)
{
  if(!sISHANDLE(handle)) return;
#if !defined(_WIN32)
  glBindTexture(GL_TEXTURE_2D,Textures[handle].GLTex);
  glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_SHORT_4_4_4_4,data);
#endif
}

void sSystem_::FlushTexture(sInt handle) { (void)handle; }

/****************************************************************************/
/***                                                                      ***/
/***   Geometry (VBO/IBO with client-array fallback)                      ***/
/***                                                                      ***/
/****************************************************************************/

void sGeoBuffer::Init()
{
  Type = 0; Size = 0; Used = 0; UserCount = 0;
#if !defined(_WIN32)
  GLVBO = 0;
#endif
}

sBool sGeoBuffer::Alloc(sInt count,sInt size,sInt &first,sInt type)
{
  sInt bytes = count*size;
  first = (Used + 15) & ~15;                 // 16-byte alignment like the DX port
  if(first + bytes > Size) return sFALSE;
  Used = first + bytes;
  return sTRUE;
}

void sGeoBuffer::Free() {}

sInt sSystem_::GeoAdd(sInt fvf,sInt prim)
{
  for(sInt h = 1;h < MAX_GEOHANDLE;h++)
  {
    if(GeoHandle[h].Mode) continue;
    GeoHandle[h].Mode = prim;
    GeoHandle[h].FVF = fvf;
    GeoHandle[h].VertexSize = 0;
    GeoHandle[h].Locked = 0;
    return h;
  }
  return sINVALID;
}

void sSystem_::GeoRem(sInt handle)
{
  if(!sISHANDLE(handle)) return;
  GeoHandle[handle].Mode = 0;
}

void sSystem_::GeoFlush() {}
void sSystem_::GeoFlush(sInt handle,sInt what) { (void)handle;(void)what; }

sInt sSystem_::GeoDraw(sInt &handle)
{
  handle = handle;                            // caller keeps same handle
  return sGEO_VERTEX|sGEO_INDEX;              // always re-upload (simple path)
}

// Vertex formats matching sFVF_* in _start.hpp
static void SetClientArrays(sInt fvf,const sF32 *fp)
{
  switch(fvf)
  {
  case sFVF_STANDARD:                         // pos,normal,tex0 (32B stride used as 28B struct)
    glVertexPointer(3,GL_FLOAT,0,fp+0);
    glNormalPointer(GL_FLOAT,0,fp+3);
    glTexCoordPointer(2,GL_FLOAT,0,fp+6);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    break;
  case sFVF_COMPACT:                          // pos,color
    glVertexPointer(3,GL_FLOAT,0,fp+0);
    glColorPointer(4,GL_UNSIGNED_BYTE,0,(const GLubyte *)(fp+3));
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    break;
  case sFVF_DOUBLE:                           // pos,color,tex0,tex1
    glVertexPointer(3,GL_FLOAT,0,fp+0);
    glColorPointer(4,GL_UNSIGNED_BYTE,0,(const GLubyte *)(fp+3));
    glTexCoordPointer(2,GL_FLOAT,0,fp+4);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    break;
  default:                                    // TSPACE variants: position only for now
    glVertexPointer(3,GL_FLOAT,0,fp+0);
    glEnableClientState(GL_VERTEX_ARRAY);
    break;
  }
}

void sSystem_::GeoBegin(sInt handle,sInt vc,sInt ic,sF32 **fp,void **ip,sInt upd)
{
  sGeoHandle &gh = GeoHandle[handle];
  static sF32 *scratchV = 0;
  static sU16 *scratchI = 0;
  static int scratchVSize = 0,scratchISize = 0;

  sInt vbytes = vc * 16 * sizeof(sF32);       // worst-case vertex footprint
  if(vbytes > scratchVSize)
  {
    free(scratchV);
    scratchV = (sF32 *)malloc(vbytes);
    scratchVSize = vbytes;
  }
  sInt ibytes = ic * sizeof(sU16);
  if(ibytes > scratchISize)
  {
    free(scratchI);
    scratchI = (sU16 *)malloc(ibytes?ibytes:4);
    scratchISize = ibytes?ibytes:4;
  }
  GeoScratchV = scratchV;
  GeoScratchI = (sU16 *)scratchI;
  *fp = scratchV;
  *ip = scratchI;
  CurDrawVC = vc;
  CurDrawIC = ic;
  (void)upd;
  (void)gh;
}

void sSystem_::GeoEnd(sInt handle,sInt vc,sInt ic)
{
  if(vc < 0) vc = CurDrawVC;
  if(ic < 0) ic = CurDrawIC;
  sGeoHandle &gh = GeoHandle[handle];

  sF32 *curV = GeoScratchV; sU16 *curI = GeoScratchI;

  GLenum mode = GL_TRIANGLES;
  switch(gh.Mode & 7)
  {
  case sGEO_POINT:     mode = GL_POINTS;          break;
  case sGEO_LINE:      mode = GL_LINES;           break;
  case sGEO_TRI:       mode = GL_TRIANGLES;       break;
  case sGEO_LINESTRIP: mode = GL_LINE_STRIP;      break;
  case sGEO_TRISTRIP:  mode = GL_TRIANGLE_STRIP;  break;
  case sGEO_QUAD:      mode = GL_QUADS;           break;
  }

  SetClientArrays(gh.FVF,curV);
  if(ic)
    glDrawElements(mode,ic,GL_UNSIGNED_SHORT,curI);
  else if(vc)
    glDrawArrays(mode,0,vc);
  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_NORMAL_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);

  // perf counters
  PerfThis.Triangle += ic/3;
  PerfThis.Vertex += vc;
}

/****************************************************************************/
/***                                                                      ***/
/***   Material system (state stream -> GL fixed function)                ***/
/***                                                                      ***/
/****************************************************************************/

sInt sSystem_::MtrlAddSetup(const sU32 *states,const sU32 *vs,const sU32 *ps)
{
  for(sInt i = 0;i < MAX_SETUPS;i++)
  {
    if(Setups[i].RefCount) continue;
    Setups[i].RefCount = 1;
    Setups[i].States = (sU32 *)states;
    Setups[i].VSCode = (sU32 *)vs;
    Setups[i].PSCode = (sU32 *)ps;
    // TODO(R2 milestone 2): transpile DXBC vs_3_0/ps_3_0 bytecode to GLSL
    // via the existing shadercompile.cpp AST and create a GL program object.
    // For now setups are recorded; MtrlSetSetup applies the state stream only.
    return i;
  }
  return sINVALID;
}

void sSystem_::MtrlAddRefSetup(sInt sid) { if(sid>=0) Setups[sid].RefCount++; }

void sSystem_::MtrlRemSetup(sInt sid) { if(sid>=0 && --Setups[sid].RefCount<=0) Setups[sid].Cleanup(); }

void sMaterialSetup::Cleanup()
{
  RefCount = 0; States = 0; VSCode = 0; PSCode = 0;
}

void sSystem_::MtrlClearCaches() {}

void sSystem_::MtrlSetSetup(sInt sid)
{
  // Walk the packed render-state stream and apply an approximation of each
  // D3D state to OpenGL fixed function. Unknown tokens are skipped so that
  // content built for the DX backend still renders reasonable output.
  if(sid < 0) return;
  CurrentSetupId = sid;
  sU32 *stream = Setups[sid].States;
  if(!stream) return;
  SetStates(stream);
}

void sSystem_::SetStates(sU32 *stream)
{
  while(*stream)
  {
    sU32 token = *stream++ & 0x1ff;
    sU32 value = *stream++;
    SetState(token,value);
  }
}

void sSystem_::SetState(sU32 token,sU32 value)
{
  switch(token)
  {
    // DirectX9 D3DSRCBLEND/D3DDESTBLEND approximations
  case 0x0b3:                                   // alpha blend enable-ish
    if(value) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    break;
  case 0x0a4:                                   // zfunc
    {
      static const GLenum zf[8]={GL_NEVER,GL_LESS,GL_EQUAL,GL_LEQUAL,
                                 GL_GREATER,GL_NOTEQUAL,GL_GEQUAL,GL_ALWAYS};
      glDepthFunc(zf[value&7]);
    }
    break;
  case 0x0a2:                                   // zwriteenable
    glDepthMask(value?GL_TRUE:GL_FALSE);
    break;
  case 0x0a7:                                   // cullmode
    if(value==1) glDisable(GL_CULL_FACE);
    else { glEnable(GL_CULL_FACE); glCullFace(GL_BACK); }
    break;
  case 0x0ac:                                   // lighting off flag from states
    if(value) glEnable(GL_LIGHTING); else glDisable(GL_LIGHTING);
    break;
  default:
    break;                                      // ignore unknown DX states for now
  }
}

void sSystem_::MtrlSetInstance(const sMaterialInstance &inst)
{
  for(sInt i = 0;i < inst.NumTextures;i++)
  {
    glActiveTexture(GL_TEXTURE0+i);
    glEnable(GL_TEXTURE_2D);
    if(inst.Textures[i]>=0)
      glBindTexture(GL_TEXTURE_2D,Textures[inst.Textures[i]].GLTex);
  }
  // constants: keep last for future GLSL uniform upload
  NumVSConstantsCache = inst.NumVSConstants;
  VSConstantsCache = inst.VSConstants;
  NumPSConstantsCache = inst.NumPSConstants;
  PSConstantsCache = inst.PSConstants;
}

// Engine matrices are row-major (i,j,k,l basis rows). OpenGL expects
// column-major arrays, so we transpose while copying.
static void LoadRowMajorGL(const sMatrix &m)
{
  const sF32 *r = (const sF32 *)&m;   // [i.x i.y i.z i.w | j... | k... | l...]
  GLfloat c[16] = {
    r[0], r[4], r[8],  r[12],
    r[1], r[5], r[9],  r[13],
    r[2], r[6], r[10], r[14],
    r[3], r[7], r[11], r[15] };
  glLoadMatrixf(c);
}

static void MultRowMajorGL(const sMatrix &m)
{
  const sF32 *r = (const sF32 *)&m;
  GLfloat c[16] = {
    r[0], r[4], r[8],  r[12],
    r[1], r[5], r[9],  r[13],
    r[2], r[6], r[10], r[14],
    r[3], r[7], r[11], r[15] };
  glMultMatrixf(c);
}

void sSystem_::SetViewProject(const sMaterialEnv *env)
{
  sMatrix proj,caminv;
  env->MakeProjectionMatrix(proj);
  caminv.Invert(env->CameraSpace);

  glMatrixMode(GL_PROJECTION);
  LoadRowMajorGL(proj);
  glMatrixMode(GL_MODELVIEW);
  LoadRowMajorGL(caminv);
  MultRowMajorGL(env->ModelSpace);
  LastCamera = env->CameraSpace;
  LastMatrix = env->ModelSpace;
  LastProjection = proj;
}

void sSystem_::GetTransform(sInt mode,sMatrix &mat)
{
  switch(mode)
  {
  case sGT_VIEW:      mat = LastCamera;                     break;
  case sGT_MODELVIEW: mat.Mul3(LastCamera,LastMatrix); break;
  case sGT_MODEL:     mat = LastMatrix;                     break;
  case sGT_PROJECT:   mat = LastProjection;                 break;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   2D overlay paint buffer                                            ***/
/***                                                                      ***/
/****************************************************************************/

void sSystem_::Begin2D(sU32 **buffer,sInt &stride)
{
  sInt w = ViewportX,h = ViewportY;
  if(!PaintBufMem || PaintW != w || PaintH != h)
  {
    free(PaintBufMem);
    PaintBufMem = (sU32 *)calloc((size_t)w*h,4);
    PaintW = w; PaintH = h;
  }
  *buffer = PaintBufMem;
  stride = w;
}

static void gluOrtho2Dish(sInt w,sInt h)
{
  GLdouble m[16] = { 2.0/w,0,0,0, 0,-2.0/h,0,0, 0,0,-1,0, -1,1,0,1 };
  glMultMatrixd(m);
}

void sSystem_::End2D()
{
  if(!PaintW) return;
  // draw software framebuffer as fullscreen textured quad
  GLuint tex;
  glGenTextures(1,&tex);
  glBindTexture(GL_TEXTURE_2D,tex);
  glPixelStorei(GL_UNPACK_ALIGNMENT,4);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,PaintW,PaintH,0,GL_BGRA,
               GL_UNSIGNED_BYTE,PaintBufMem);
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_DEPTH_TEST);
  glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
  gluOrtho2Dish(PaintW,PaintH);
  glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
  glBegin(GL_QUADS);
    glTexCoord2f(0,1); glVertex2f(0,0);
    glTexCoord2f(1,1); glVertex2f((float)PaintW,0);
    glTexCoord2f(1,0); glVertex2f((float)PaintW,(float)PaintH);
    glTexCoord2f(0,0); glVertex2f(0,(float)PaintH);
  glEnd();
  glMatrixMode(GL_PROJECTION); glPopMatrix();
  glMatrixMode(GL_MODELVIEW); glPopMatrix();
  glEnable(GL_DEPTH_TEST);
  glDeleteTextures(1,&tex);
}


/****************************************************************************/
/***                                                                      ***/
/***   Audio (DirectSound -> SDL audio callback)                          ***/
/***                                                                      ***/
/****************************************************************************/

static sSoundHandler gSoundHandler = 0;
static void *gSoundUser = 0;
static sInt gSampleTime = 0;

static void SdlAudioCallback(void *userdata,Uint8 *stream,int len)
{
  if(gSoundHandler)
    gSoundHandler((sS16 *)stream,len/(2*sizeof(sS16)),gSoundUser);
  else
    memset(stream,0,len);
  gSampleTime += len/(2*sizeof(sS16));
}

void sSystem_::SetSoundHandler(sSoundHandler hnd,sInt align,void *user)
{
  gSoundHandler = hnd;
  gSoundUser = user;
  static sBool opened = sFALSE;
  if(!opened)
  {
    SDL_AudioSpec want,have;
    SDL_zero(want);
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 2048;
    want.callback = SdlAudioCallback;
    if(SDL_OpenAudioDevice(0,0,&want,&have,0) == 0)
      fprintf(stderr,"SDL audio unavailable: %s\n",SDL_GetError());
    SDL_PauseAudio(0);
    opened = sTRUE;
  }
  (void)align;
}

sInt sSystem_::GetCurrentSample() { return gSampleTime; }

sBool sSystem_::InitDS() { return sTRUE; }    // kept for API compatibility
void sSystem_::ExitDS() {}
void sSystem_::FillDS() {}
void sSystem_::MarkDS() {}

/****************************************************************************/
/***                                                                      ***/
/***   Input helpers                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void sSystem_::GetInput(sInt id,sInputData &data)
{
  memset(&data,0,sizeof(data));
  if(id == 0)                                  // mouse as absolute "tablet"
  {
    int x,y;
    Uint32 b = SDL_GetMouseState(&x,&y);
    data.Type = sIDT_MOUSE;
    data.AnalogCount = 2;
    data.DigitalCount = 3;
    data.Analog[0] = x * 0x10000 / (ViewportX?ViewportX:1);
    data.Analog[1] = y * 0x10000 / (ViewportY?ViewportY:1);
    data.Digital = (b&SDL_BUTTON_LMASK?1:0)|(b&SDL_BUTTON_MMASK?2:0)|
                   (b&SDL_BUTTON_RMASK?4:0);
  }
  else
    data.Type = sIDT_NONE;
}

sU32 sSystem_::GetKeyboardShiftState()
{
  Uint16 mod = SDL_GetModState();
  sU32 q = 0;
  if(mod & KMOD_LSHIFT) q |= sKEYQ_SHIFTL;
  if(mod & KMOD_RSHIFT) q |= sKEYQ_SHIFTR;
  if(mod & KMOD_LCTRL)  q |= sKEYQ_CTRLL;
  if(mod & KMOD_RCTRL)  q |= sKEYQ_CTRLR;
  if(mod & KMOD_ALT)    q |= sKEYQ_ALT;
  if(mod & KMOD_CAPS)   q |= sKEYQ_CAPS;
  return q;
}

void sSystem_::GetKeyName(sChar *buffer,sInt size,sU32 key)
{
  snprintf(buffer,size,"%s",SDL_GetScancodeName((SDL_Scancode)key));
}

sBool sSystem_::GetAbortKey() { return WAbortKeyFlag ? sTRUE : sFALSE; }
void sSystem_::ClearAbortKey() { WAbortKeyFlag = 0; }

sBool sSystem_::GetWinMouse(sInt &x,sInt &y)
{
  int wx,wy;
  SDL_GetMouseState(&wx,&wy);
  x = wx; y = wy;
  return sTRUE;
}

sBool sSystem_::GetWinMouseAbs(sInt &x,sInt &y)
{
  SDL_GetGlobalMouseState(&x,&y);
  return sTRUE;
}

void sSystem_::SetWinMouse(sInt x,sInt y)
{
  SDL_WarpMouseInWindow((SDL_Window*)(uintptr_t)Screen[0].Window,x,y);
}

void sSystem_::HideWinMouse(sBool hide)
{
  SDL_ShowCursor(hide ? SDL_DISABLE : SDL_ENABLE);
}

void sSystem_::SetWinTitle(sChar *name)
{
  SDL_SetWindowTitle((SDL_Window*)(uintptr_t)Screen[0].Window,name);
}

void sSystem_::MoveWindow(sInt dx,sInt dy)
{
  SDL_Window *w = (SDL_Window*)(uintptr_t)Screen[0].Window;
  int x,y; SDL_GetWindowPosition(w,&x,&y);
  SDL_SetWindowPosition(w,x+dx,y+dy);
}

void sSystem_::SetWinMode(sInt mode)
{
  SDL_Window *w = (SDL_Window*)(uintptr_t)Screen[0].Window;
  switch(mode)
  {
  case 0: SDL_RestoreWindow(w); break;
  case 1: SDL_MaximizeWindow(w); break;
  case 2: SDL_MinimizeWindow(w); break;
  }
}

sInt sSystem_::GetWinMode()
{
  Uint32 f = SDL_GetWindowFlags((SDL_Window*)(uintptr_t)Screen[0].Window);
  if(f & SDL_WINDOW_MAXIMIZED) return 1;
  if(f & SDL_WINDOW_MINIMIZED) return 2;
  return 0;
}

void sSystem_::Reset(sU32 flags,sInt x,sInt y,sInt x2,sInt y2)
{
  ConfigFlags = flags; ConfigX = x; ConfigY = y;
  ExitScreens();
  InitScreens();
  KeyBufferPush(sKEY_MODECHANGE);
  (void)x2;(void)y2;
}

/****************************************************************************/
/***                                                                      ***/
/***   Filesystem (POSIX replaces Win32 file APIs)                        ***/
/***                                                                      ***/
/****************************************************************************/

sU8 *sSystem_::LoadFile(const sChar *name,sInt &size)
{
  FILE *f = fopen(name,"rb");
  if(!f) { size = 0; return 0; }
  fseek(f,0,SEEK_END);
  size = (sInt)ftell(f);
  fseek(f,0,SEEK_SET);
  sU8 *d = (sU8 *)malloc(size+1);
  if(fread(d,1,size,f) != (size_t)size) { fclose(f); free(d); size=0; return 0; }
  fclose(f);
  d[size] = 0;
  return d;
}

sU8 *sSystem_::LoadFile(const sChar *name)
{
  sInt sz;
  return LoadFile(name,sz);
}

sU8 *sSystem_::LoadFileIfNewerThan(const sChar *name,const sChar *other,sInt &size)
{
  struct stat a,b;
  if(stat(name,&a) != 0) { size = 0; return 0; }
  if(stat(other,&b) == 0 && b.st_mtime >= a.st_mtime) { size = 0; return 0; }
  return LoadFile(name,size);
}

sChar *sSystem_::LoadText(const sChar *name)
{
  return (sChar *)LoadFile(name);
}

sBool sSystem_::SaveFile(const sChar *name,const sU8 *data,sInt size)
{
  FILE *f = fopen(name,"wb");
  if(!f) return sFALSE;
  sBool ok = fwrite(data,1,size,f) == (size_t)size;
  fclose(f);
  return ok ? sTRUE : sFALSE;
}

sU64 sSystem_::GetFileStamp(const sChar *name)
{
  struct stat st;
  if(stat(name,&st) != 0) return 0;
  return (sU64)st.st_mtime * 1000000ULL + (sU64)st.st_size;
}

sDirEntry *sSystem_::LoadDir(const sChar *name)
{
  DIR *d = opendir(name);
  if(!d) return 0;
  sInt cap = 256,count = 0;
  sDirEntry *ents = (sDirEntry *)malloc(cap*sizeof(sDirEntry));
  struct dirent *e;
  while((e = readdir(d)) != 0)
  {
    if(count+2 > cap) { cap*=2; ents=(sDirEntry*)realloc(ents,cap*sizeof(sDirEntry)); }
    snprintf(ents[count].Name,256,"%s",e->d_name);
    struct stat st;
    char path[1024];
    snprintf(path,1024,"%s/%s",name,e->d_name);
    stat(path,&st);
    ents[count].IsDir = S_ISDIR(st.st_mode);
    ents[count].Size = ents[count].IsDir ? 0 : (sInt)st.st_size;
    count++;
  }
  closedir(d);
  ents[count].Name[0] = 0;
  ents[count].Size = 0;
  ents[count].IsDir = sFALSE;
  return ents;
}

sBool sSystem_::MakeDir(const sChar *name) { return mkdir(name,0755)==0 ? sTRUE:sFALSE; }
sBool sSystem_::CheckDir(const sChar *name)
{
  struct stat st; return (stat(name,&st)==0 && S_ISDIR(st.st_mode)) ? sTRUE:sFALSE;
}
sBool sSystem_::CheckFile(const sChar *name)
{
  struct stat st; return (stat(name,&st)==0) ? sTRUE:sFALSE;
}
sBool sSystem_::RenameFile(const sChar *o,const sChar *n) { return rename(o,n)==0 ? sTRUE:sFALSE; }
sBool sSystem_::DeleteFile(const sChar *name) { return remove(name)==0 ? sTRUE:sFALSE; }

void sSystem_::GetCurrentDir(sChar *buffer,sInt size)
{
  if(!getcwd(buffer,size)) snprintf(buffer,size,"./");
}

sU32 sSystem_::GetDriveMask() { return 0x00000001; }  // single POSIX root fs

sBool sSystem_::FileRequester(sChar *buffer,sInt size,sU32 flags)
{
  // GUI file dialogs are part of the Windows-only werkkzeug editor; the
  // open-source player build never calls this. Graceful stub:
  (void)flags;
  buffer[0] = 0;
  return sFALSE;
}

sBitmap *sSystem_::LoadBitmap(const sU8 *data,sInt size)
{
  (void)data;(void)size;
  return 0;                                       // use pngloader/loadpng.cpp instead
}

sBool sSystem_::LoadBitmapCore(const sU8 *data,sInt size,sInt &x,sInt &y,sU8 *&d)
{
  (void)data;(void)size;(void)x;(void)y;(void)d;
  return sFALSE;
}

/****************************************************************************/
/***                                                                      ***/
/***   Misc API stubs needed to link the player subset                    ***/
/***                                                                      ***/
/****************************************************************************/

void sSystem_::Log(sChar *msg) { fprintf(stderr,"%s\n",msg); }

void sSystem_::Abort(sChar *msg)
{
  fprintf(stderr,"ABORT: %s\n",msg);
  exit(2);
}

sChar *sSystem_::GetCmdLine() { return WCmdLineBuf; }

#if sPLAYER
void sSystem_::WaitForKey()
{
  SDL_Event ev;
  while(!SDL_WaitEvent(&ev)) {}
}

void sSystem_::Progress(sInt done,sInt max) { (void)done;(void)max; }
#endif

void sSystem_::GrabScreen(sInt sourceRT,const sRect &win,sInt tex,sBool stretch)
{ (void)sourceRT;(void)win;(void)tex;(void)stretch; }

void sSystem_::SetScissor(const sFRect *scissor) { (void)scissor; }

void sSystem_::GetPerf(sPerfInfo &info,sInt mode)
{
  (void)mode;
  info = PerfLast;
}

void sSystem_::PrintShader(const sU32 *shader) { (void)shader; }

/****************************************************************************/
/***                                                                      ***/
/***   Viewport                                                           ***/
/***                                                                      ***/
/****************************************************************************/

// Portable replacement for the sViewport methods that previously lived only
// in the Direct3D backend (_start.cpp, _WIN32-only).

void sViewport::Init(sInt screen)
{
  Screen = screen;
  RenderTarget = -1;
  Window.Init(0,0,sSystem->Screen[screen].XSize,sSystem->Screen[screen].YSize);
}

void sViewport::InitTex(sInt handle)
{
  Screen = -1;
  RenderTarget = handle;
  Window.x0 = 0;
  Window.y0 = 0;
  sSystem->GetTextureSize(handle,Window.x1,Window.y1);
}

/****************************************************************************/

sBool sSystem_::VideoStart(sChar *filename,sF32 fpsrate,sInt xRes,sInt yRes)
{ (void)filename;(void)fpsrate;(void)xRes;(void)yRes; return sFALSE; }
void sSystem_::VideoViewport(sViewport &vp) { (void)vp; }
sInt sSystem_::VideoWriteFrame(sF32 &u1,sF32 &v1) { (void)u1;(void)v1; return sINVALID; }
void sSystem_::VideoEnd() {}

// Host font: minimal bitmap-less implementation (editor-only feature otherwise)
sInt sSystem_::FontBegin(sInt pagex,sInt pagey,const sChar *name,sInt xs,sInt ys,sInt style)
{ (void)pagex;(void)pagey;(void)name;(void)xs;(void)ys;(void)style; return ys; }
sInt sSystem_::FontWidth(const sChar *string,sInt count)
{ sInt n = count<0?(sInt)strlen(string):count; return n*(ViewportY/40+1); }
void sSystem_::FontCharWidth(sInt ch,sInt *widths) { widths[0]=widths[1]=8; (void)ch; }
void sSystem_::FontPrint(sInt x,sInt y,const sChar *string,sInt count)
{ (void)x;(void)y;(void)string;(void)count; }
void sSystem_::FontPrint(sInt x,sInt y,const sU16 *string,sInt count)
{ (void)x;(void)y;(void)string;(void)count; }
void sSystem_::FontEnd() {}

// 3D sound (kkrieger-linked API): simplified - volume/pan ignored, mono queue
#if sLINK_KKRIEGER
sInt sSystem_::SampleAdd(sS16 *data,sInt size,sInt buffers,sInt usehandle,sBool is3d)
{ (void)data;(void)size;(void)buffers;(void)is3d; return usehandle>=0?usehandle:1; }
void sSystem_::SampleRem(sInt handle) { (void)handle; }
void sSystem_::SampleRemAll() {}
sInt sSystem_::SamplePlay(sInt handle,sF32 volume,sF32 pan,sInt freq)
{ (void)handle;(void)volume;(void)pan;(void)freq; return 0; }
void sSystem_::Sample3DParam(sInt handle,sInt buffer,const sVector &pos,const sVector &vel,
                             sF32 minDist,sF32 maxDist)
{ (void)handle;(void)buffer;(void)pos;(void)vel;(void)minDist;(void)maxDist; }
void sSystem_::Sample3DListener(const sVector &pos,const sVector &vel,const sVector &up,
                                const sVector &fwd,sF32 doppler,sF32 rolloff)
{ (void)pos;(void)vel;(void)up;(void)fwd;(void)doppler;(void)rolloff; }
void sSystem_::Sample3DCommit() {}
#endif

// DirectInput replacements (not needed; SDL events feed KeyBuffer directly)
sBool sSystem_::InitDI() { return sTRUE; }
void sSystem_::ExitDI() {}
void sSystem_::PollDI() {}
void sSystem_::AddAKey(sU32 *Scans,sInt ascii) { (void)Scans;(void)ascii; }

void sSystem_::CreateGeoBuffer(sInt i,sInt dyn,sInt index,sInt size)
{
  sGeoBuffer &gb = GeoBuffer[i];
  gb.Init();
  gb.Size = size;
#if !defined(_WIN32)
  if(pGlGenBuffers && pGlBindBuffer && pGlBufferData)
  {
    pGlGenBuffers(1,&gb.GLVBO);
    pGlBindBuffer(index?GL_ELEMENT_ARRAY_BUFFER:GL_ARRAY_BUFFER,gb.GLVBO);
    pGlBufferData(index?GL_ELEMENT_ARRAY_BUFFER:GL_ARRAY_BUFFER,size,0,
                  dyn?GL_STREAM_DRAW:GL_STATIC_DRAW);
    pGlBindBuffer(index?GL_ELEMENT_ARRAY_BUFFER:GL_ARRAY_BUFFER,0);
  }
#endif
  (void)dyn;(void)index;
}

void sSystem_::AllocBufferInternal(sGeoBufferRef &ref,sInt size,sBool stat,sInt index,void **ptr)
{
  ref.Buffer = 0; ref.Start = 0; ref.Count = 0;
  *ptr = 0;                                   // simple path uses scratch buffers
  (void)size;(void)stat;(void)index;
}

void sSystem_::ReCreateZBuffer() {}
void sSystem_::MakeCubeNormalizer() {}
void sSystem_::MakeAttenuationVolume() {}

/****************************************************************************/
/***                                                                      ***/
/***   main() - POSIX entry point replacing WinMain                       ***/
/***                                                                      ***/
/****************************************************************************/

int main(int argc,char **argv)
{
  // join argv into the cmd-line string the engine expects
  sInt pos = 0;
  for(int i = 1;i < argc && pos < (int)sizeof(WCmdLineBuf)-2;i++)
  {
    if(i > 1) WCmdLineBuf[pos++] = ' ';
    pos += snprintf(WCmdLineBuf+pos,sizeof(WCmdLineBuf)-pos,"%s",argv[i]);
  }
  WCmdLineBuf[pos] = 0;

  if(sAppHandler(sAPPCODE_CONFIG,0))          // app calls sSetConfig() here
  {
    if(sSystem)
    {
      sSystem->InitX();                       // create window/GL ctx + first frame
      sSystem->RunLoop();                     // SDL event pump until quit
      delete sSystem;
      sSystem = 0;
    }
  }

  SDL_Quit();
  return 0;
}

#endif // !defined(_WIN32)
