# Makefile for Linux / open-source build
# All MSVC keywords (__forceinline, __cdecl, __stdcall, __declspec, __int64)
# have been replaced with portable GCC/C++11 equivalents in the sources.
CXX = g++
CXXFLAGS = -std=c++11 -O2 -Wall -Wno-unused -Is. -Imaterials -Iwerkkzeug3_kkrieger -Iwerkkzeug3_kkrieger/pngloader
LDFLAGS =

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

CORE_OBJECTS = $(CORE_SOURCES:.cpp=.o)

EXECUTABLE = kkrieger_linux

all: core

# Compile-only verification of the portable core engine files
core: $(CORE_OBJECTS)
	@echo "Core engine files compiled successfully on Linux (open-source toolchain)."

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(EXECUTABLE): $(CORE_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(CORE_OBJECTS) $(LDFLAGS)

clean:
	rm -f $(CORE_OBJECTS) $(EXECUTABLE)

.PHONY: all core clean
