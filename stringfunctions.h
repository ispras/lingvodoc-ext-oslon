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

void* _addcalc(float*d, LPTSTR s)
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


LPTSTR strcpyi(LPTSTR a, int i)
{
	_ltow(i, a, 10);
	return a;
}
LPTSTR strcpyh(LPTSTR a, int i, int szpad = 0)
{
	if (!szpad)
	{
		_ltow(i, a, 16);
	}
	else
	{
		TCHAR _buf[20];
		//string chup=_strupr(_ltoa(i,_buf,16));
		_ltow(i, _buf, 16);
		int sz = lstrlen(_buf);

		for (int i = 0; i < szpad - sz; i++)
			a[i] = L'0';

		int end = szpad - sz;

		a[end] = '\0';
		lstrcpy(a + end, _buf);
	}

	return a;
}
void strcati(LPTSTR a, int i)
{
	int sz = lstrlen(a);
	_ltow(i, a + sz, 10);
	//return sz+lstrlen(a+sz);
}
void strcatih(LPTSTR a, void* i)
{
	int sz = lstrlen(a);
	lstrcpy(a + sz, L"0x");
	_ltow((long long)i, a + sz + 2, 0x10);
	//return sz+lstrlen(a+sz);
}
void strcatb(LPTSTR a, int b)
{
	int _sz = lstrlen(a);
	a[_sz] = b;
	a[_sz + 1] = L'\0';
}



LPTSTR StrCpyWMax(LPTSTR dest, LPTSTR src, int len)
{
	if (lstrlen(src) < len)
		return wcscpy(dest, src);
	else
	{
		dest[len - 1] = L'\0';
		return (LPTSTR)memcpy(dest, src, (len - 1) * sizeof(TCHAR));
	}
}
