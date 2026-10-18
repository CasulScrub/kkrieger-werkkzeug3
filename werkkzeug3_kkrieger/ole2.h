// Stub for ole2.h - Minimal OLE definitions
#pragma once
#include "windows.h"

// OLE interfaces and functions (stubs)
typedef struct IOleObject { virtual ~IOleObject() {} };
typedef struct IOleClientSite { virtual ~IOleClientSite() {} };

// OLE storage
typedef struct IStorage { virtual ~IStorage() {} };
typedef struct IStream { virtual ~IStream() {} };

// OLE types
typedef DWORD DWORD;
typedef WORD WORD;
