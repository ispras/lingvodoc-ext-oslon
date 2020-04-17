#pragma once

#ifdef __linux__ 
#include <cstdlib>
#include <stdio.h>
#include <wchar.h>
#endif

typedef LPTSTR string;

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

wchar_t* Hex(wchar_t* buf, void* i)
{
	lstrcpy(buf, TEXT("0x"));
	_ltow(*(int*)i, buf + 2, 0x10);
	return buf;
}

wchar_t* Hex(wchar_t* buf, int i)
{
	lstrcpy(buf, TEXT("0x"));
	_ltow(i, buf + 2, 0x10);
	//ltos?
	return buf;
}

void FreeAndZero(void*& mem)
{
	if (mem)
	{
		free(mem);
		mem = NULL;
	}
}

LPSTR StoreStringA(LPSTR str) {
	if (!str)
		return NULL;

	LPSTR ret = (LPSTR)malloc(lstrlenA(str) + 1);
	lstrcpyA(ret, str);
	return ret;
}
LPWSTR StoreStringW(LPWSTR str) {
	if (!str)
		return NULL;

	LPWSTR ret = (LPWSTR)malloc(lstrlenW(str) * 2 + 2);
	lstrcpyW(ret, str);
	return ret;
}
LPSTR StoreNonNullStringA(LPSTR str) {
	if (!str)
		return NULL;
	if (!str[0])
		return NULL;

	LPSTR ret = (LPSTR)malloc(lstrlenA(str) + 1);
	lstrcpyA(ret, str);
	return ret;
}
LPWSTR StoreNonNullStringW(LPWSTR str) {
	if (!str)
		return NULL;
	if (!str[0])
		return NULL;

	LPWSTR ret = (LPWSTR)malloc(lstrlenW(str) * 2 + 2);
	lstrcpyW(ret, str);
	return ret;
}
wchar_t* ConvertToLowercaseW(wchar_t* in, wchar_t* out)
{
	while (*in)
	{
		*out = towlower(*in);
		out++;
		in++;
	}

	//while (*in)
		//так пока не пашет!	
		//*out++ = towlower(*in++);
	*out = L'\0';
	return out;
}
char* ConvertToLowercaseA(char* in, char* out)
{
	//while (*in)
		//так пока не пашет!	
		//*out++ = tolower(*in++);
	*out = '\0';
	return out;
}
#ifdef UNICODE
#define ConvertToLowercase ConvertToLowercaseW
#define StoreString StoreStringW
#define StoreNonNullString StoreNonNullStringW
#else
#define ConvertToLowercase ConvertToLowercaseA
#define StoreString StoreStringA
#define StoreNonNullString StoreNonNullStringA
#endif


class String
{
	/////////////////////////////
public:
	/////////////////////////////
	int			size;
	LPTSTR		mem;

	bool alloc(int sz)
	{
		free();

		if (sz != -1)
		{
			size = sz;
			mem = (LPTSTR)malloc(size * 2 + 2);
			return !!mem;
		}
		return true;
	}
	bool settext(LPTSTR text)
	{
		int sz;
		if (text)
			sz = lstrlen(text);
		else
			sz = -1;

		if (!alloc(sz))
			return false;

		lstrcpy(mem, text);

		return true;
	}
	void free()
	{
		if (mem)
		{
			::free(mem);
			size = -1;
		}
	}
public:
	String()
	{
		mem = NULL;
		size = -1;
	}
	String(LPTSTR text)
	{
		settext(text);
	}
	~String()
	{
		free();
	}
	void __set(TCHAR* s)
	{
		settext(s);
	}
	inline void operator = (TCHAR* s)
	{
		settext(s);
	}
	//	inline operator bool () { return size; }

	//	inline operator LPTSTR () { return mem; }
};
