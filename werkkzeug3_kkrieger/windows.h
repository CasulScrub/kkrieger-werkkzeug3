// Stub for windows.h - Minimal Windows API definitions for PDB parsing
#pragma once

// Basic type definitions needed for pdbfile.cpp
typedef unsigned long DWORD;
typedef int BOOL;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef float FLOAT;
typedef double DOUBLE;
typedef char CHAR;
typedef unsigned char UCHAR;
typedef unsigned int UINT;
typedef long LONG;
typedef unsigned long ULONG;
typedef short SHORT;
typedef unsigned short USHORT;
typedef void* HANDLE;
typedef void* LPVOID;
typedef const void* LPCVOID;
typedef int INT;
typedef unsigned int UINT32;
typedef long LONG32;
typedef unsigned long DWORD32;
typedef unsigned long long ULONGLONG;
typedef long long LONGLONG;
typedef long HRESULT;

#define S_OK ((HRESULT)0L)
#define E_FAIL ((HRESULT)0x80004005L)
#define S_FALSE ((HRESULT)1L)
#define E_NOTIMPL ((HRESULT)0x80004001L)
#define E_OUTOFMEMORY ((HRESULT)0x8007000EL)
#define E_POINTER ((HRESULT)0x80004003L)

// GUID structure
typedef struct _GUID {
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[8];
} GUID, *LPGUID;

typedef const GUID &REFGUID;
typedef const GUID *LPCGUID;

// OLE types
typedef wchar_t OLECHAR;
typedef OLECHAR* BSTR;
typedef const OLECHAR* LPOLESTR;
typedef wchar_t* LPWSTR;
typedef const wchar_t* LPCWSTR;

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

// FILETIME structure
typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME, *PFILETIME;

typedef unsigned long DWORD;

#ifdef __cplusplus
extern "C" {
#endif

inline int MultiByteToWideChar(unsigned int CodePage, unsigned long dwFlags,
                             const char* lpMultiByteStr, int cbMultiByte,
                             wchar_t* lpWideCharStr, int cchWideChar) {
    return 0;
}

inline int WideCharToMultiByte(unsigned int CodePage, unsigned long dwFlags,
                             const wchar_t* lpWideCharStr, int cchWideChar,
                             char* lpMultiByteStr, int cbMultiByte,
                             const char* lpDefaultChar, int* lpUsedDefaultChar) {
    return 0;
}

#ifdef __cplusplus
}
#endif

// COM interfaces stubs
struct IUnknown { virtual ~IUnknown() {} virtual HRESULT QueryInterface(REFIID riid, void** ppv) = 0; };
struct IDiaDataSource { virtual ~IDiaDataSource() {} };
struct IDiaSession { virtual ~IDiaSession() {} };
struct IDiaSymbol { virtual ~IDiaSymbol() {} };
struct IDiaEnumSymbols { virtual ~IDiaEnumSymbols() {} };

inline HRESULT CoCreateInstance(REFCLSID rclsid, IUnknown* pUnkOuter,
                              DWORD dwClsContext, REFIID riid, LPVOID* ppv) {
    if (ppv) *ppv = nullptr;
    return E_FAIL;
}

// CLSID type (already used above)
typedef GUID CLSID;
typedef CLSID* LPCLSID;
typedef const CLSID& REFCLSID;
typedef const IID& REFIID;
