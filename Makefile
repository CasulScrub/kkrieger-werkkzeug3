# Makefile for Linux / open-source build
# All MSVC keywords (__forceinline, __cdecl, __stdcall, __declspec, __int64)
# have been replaced with portable GCC/C++11 equivalents in the sources.
CXX = g++
CXXFLAGS = -std=c++11 -O2 -Wall -Wno-unused -Is. -Imaterials -Iwerkkzeug3_kkrieger -Iwerkkzeug3_kkrieger/pngloader -Iwerkkzeug3_kkrieger/materials
LDFLAGS =
SDL_CFLAGS = $(shell pkg-config --cflags sdl2 2>/dev/null || echo -I/usr/include/SDL2)
SDL_LIBS = $(shell pkg-config --libs sdl2 2>/dev/null || echo -lSDL2) -lGL

# Portable core subset (Windows GUI/DirectX files removed; no MSVC keywords remain)
CORE_SOURCES = \
    werkkzeug3_kkrieger/_types.cpp \
    werkkzeug3_kkrieger/port_shims.cpp \
    werkkzeug3_kkrieger/werkops.cpp \
    werkkzeug3_kkrieger/shadercompile.cpp \
    werkkzeug3_kkrieger/shadercodegen.cpp \
    werkkzeug3_kkrieger/script.cpp \
    werkkzeug3_kkrieger/packer.cpp \
    werkkzeug3_kkrieger/pngloader/loadpng.cpp

# Engine/game sources that compile cleanly with the portable toolchain and
# supply the op/effect/bitmap tables referenced by werkops.o, plus sAppHandler.
ENGINE_SOURCES = \
    werkkzeug3_kkrieger/_diskitem.cpp \
    werkkzeug3_kkrieger/_gui.cpp \
    werkkzeug3_kkrieger/_intmath.cpp \
    werkkzeug3_kkrieger/_iob.cpp \
    werkkzeug3_kkrieger/_lekktor.cpp \
    werkkzeug3_kkrieger/_loader.cpp \
    werkkzeug3_kkrieger/_mp3.cpp \
    werkkzeug3_kkrieger/_util.cpp \
    werkkzeug3_kkrieger/_viruz2.cpp \
    werkkzeug3_kkrieger/_xsi.cpp \
    werkkzeug3_kkrieger/appbrowser.cpp \
    werkkzeug3_kkrieger/appfractal.cpp \
    werkkzeug3_kkrieger/appprefs.cpp \
    werkkzeug3_kkrieger/apptext.cpp \
    werkkzeug3_kkrieger/depacker.cpp \
    werkkzeug3_kkrieger/genblobspline.cpp \
    werkkzeug3_kkrieger/geneffect.cpp \
    werkkzeug3_kkrieger/geneffectdebris.cpp \
    werkkzeug3_kkrieger/geneffectex.cpp \
    werkkzeug3_kkrieger/genmesh.cpp \
    werkkzeug3_kkrieger/genminmesh.cpp \
    werkkzeug3_kkrieger/kkriegergame.cpp \
    werkkzeug3_kkrieger/mainplayer.cpp \
    werkkzeug3_kkrieger/novaplayer.cpp

ENGINE_OBJECTS = $(ENGINE_SOURCES:.cpp=.o)

CORE_OBJECTS = $(CORE_SOURCES:.cpp=.o)

EXECUTABLE = kkrieger_linux

RENDER_SOURCES = werkkzeug3_kkrieger/_startgl_sdl.cpp
RENDER_OBJECTS = $(RENDER_SOURCES:.cpp=.o)

all: core

# Compile-only verification of the portable core engine files
core: $(CORE_OBJECTS)
	@echo "Core engine files compiled successfully on Linux (open-source toolchain)."

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(EXECUTABLE): $(CORE_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(CORE_OBJECTS) $(LDFLAGS)

# SDL2/OpenGL backend needs extra include flags
werkkzeug3_kkrieger/_startgl_sdl.o: werkkzeug3_kkrieger/_startgl_sdl.cpp
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) -c $< -o $@

# Full render build: portable core + SDL2/OpenGL backend -> runnable ELF
render: $(CORE_OBJECTS) $(ENGINE_OBJECTS) $(RENDER_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(EXECUTABLE) $(CORE_OBJECTS) $(ENGINE_OBJECTS) $(RENDER_OBJECTS) $(LDFLAGS) $(SDL_LIBS) -lpng -lz -lm
	@echo "Linked $(EXECUTABLE) against libSDL2/libGL (open-source rendering stack)."

clean:
	rm -f $(CORE_OBJECTS) $(ENGINE_OBJECTS) $(RENDER_OBJECTS) $(EXECUTABLE)

.PHONY: all core render clean
