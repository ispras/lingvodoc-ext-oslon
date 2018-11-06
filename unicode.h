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

void reverse(wchar_t s[])
{
	int i, j;
	char c;

	for (i = 0, j = lstrlen(s) - 1; i < j; i++, j--) {
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}

void _ltow(int n, wchar_t s[], int radix) //здесь radix всегда = 10
{
	int i, sign;

	if ((sign = n) < 0)  /* record sign */
		n = -n;          /* make n positive */
	i = 0;
	do {       /* generate digits in reverse order */
		s[i++] = n % 10 + '0';   /* get next digit */
	} while ((n /= 10) > 0);     /* delete it */
	if (sign < 0)
		s[i++] = '-';
	s[i] = '\0';
	reverse(s);
}