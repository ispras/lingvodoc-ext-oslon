#pragma once


#ifdef __linux__ 
#include <cstdlib>
#include <stdio.h>
#include <wchar.h>


void reverse(wchar_t s[])
{
	int i, j;
	wchar_t c;

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
	s[i] = L'\0';
	reverse(s);
}

void _addcalc(float*d, LPTSTR s)
{
	*d += wcstod(s, NULL);
}
LPTSTR _donecalc(float*d, LPTSTR s, int _n)
{
	float mean = *d / _n;

	swprintf(s, 20, L"%.3f", mean);

	return s;
}

#endif

int abs(int i)
{
	if (i < 0) return -i;
	return i;
}

int pow(int _i, int p)
{
	int i = _i;
	while (p > 1)
	{
		i *= _i;
		p--;
	}
	return i;
}



void JustCopySymbols(LPTSTR* bIn, LPTSTR *bOut, int szToCopy)
{
	wcsncpy(*bOut, *bIn, szToCopy);
	*bOut += szToCopy;
	*bIn += szToCopy;
}

void ReplaceSymbols(LPTSTR* bIn, LPTSTR *bOut, int szToReplace, LPTSTR bReplaceBy)
{
	int szToReplaceBy = wcslen(bReplaceBy);

	switch (bReplaceBy[0])
	{
	case L'@':
		szToReplaceBy = 0;
		break;
	default:
		wcscpy(*bOut, bReplaceBy);
	}

	*bOut += szToReplaceBy;
	*bIn += szToReplace;
}

