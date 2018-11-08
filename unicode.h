#pragma once
#define UNICODE

typedef unsigned char byte;
typedef wchar_t WCHAR;

typedef char *LPSTR;
typedef const char *LPCSTR;
typedef WCHAR *LPWSTR;
typedef const WCHAR *LPCWSTR;

#ifdef UNICODE
typedef WCHAR TCHAR;
#else
typedef char TCHAR;
#endif

typedef TCHAR *LPTSTR;
typedef const TCHAR *LPCTSTR;

#define lstrcatW wcscat
#define lstrcatA strcat
#define lstrcat wcscat

#define lstrcpyW wcscpy
#define lstrcpyA strcpy
#define lstrcpy wcscpy

#define lstrcmpW wcscmp
#define lstrcmpA strcmp
#define lstrcmp wcscmp


#define lstrlenW wcslen
#define lstrlenA strlen
#define lstrlen wcslen

#define TEXT(x) L ##x

#include <wchar.h>
