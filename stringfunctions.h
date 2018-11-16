#pragma once

LPTSTR StrCpyWMax(LPTSTR dest, LPTSTR src, int len)
{
	if (lstrlen(src) < len)
		return wcscpy(dest, src);
	else
	{
		dest[len - 1] = '\0';
		return (LPTSTR)memcpy(dest, src, (len - 1) * sizeof(TCHAR));
	}
}


#ifdef __linux__ 

#include <stdio.h>
#include <wchar.h>
#include <cstdlib>

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

void* _addcalc(float*d,LPTSTR s)
{
	*d += wcstod(s,NULL);
}
LPTSTR _donecalc(float*d,LPTSTR s, int _n)
{
	float mean = *d / _n;

	wprintf(s, "%.3f", mean);

	return s;
}

#endif