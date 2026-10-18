# Makefile for Linux build (after removing Windows-specific files)
CXX = g++
CXXFLAGS = -std=c++11 -O2 -Wall -Wno-unused -Is. -Imaterials -Iwerkkzeug3_kkrieger

# Core source files (Windows-specific files already removed)
SOURCES = \
    werkkzeug3_kkrieger/werkkzeug.cpp \
    werkkzeug3_kkrieger/werkops.cpp \
    werkkzeug3_kkrieger/shadercompile.cpp \
    werkkzeug3_kkrieger/shadercodegen.cpp \
    werkkzeug3_kkrieger/script.cpp \
    werkkzeug3_kkrieger/ronan.cpp \
    werkkzeug3_kkrieger/pdbfile.cpp \
    werkkzeug3_kkrieger/packer.cpp \
    werkkzeug3_kkrieger/novaplayer.cpp \
    werkkzeug3_kkrieger/pngloader/loadpng.cpp \
    werkkzeug3_kkrieger/werkkzeug3/wz3_main.cpp \
    werkkzeug3_kkrieger/werkkzeug3/geneffectcubes.cpp

EXECUTABLE = kkrieger_linux

all: $(EXECUTABLE)

$(EXECUTABLE): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(EXECUTABLE)

.PHONY: all clean
