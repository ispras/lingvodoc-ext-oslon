#include <wchar.h>
#include <string.h>
#include <wctype.h>
#include <stdlib.h>
typedef LPTSTR string;

/*
wchar_t* Hex(wchar_t* buf, void* i)
{
	lstrcpy(buf, TEXT("0x"));
	_ltow(*(int*)i, buf + 2, 0x10);
//    _ltow((int)i, buf + 2, 0x10);
	return buf;
}

wchar_t* Hex(wchar_t* buf, int i)
{
	lstrcpy(buf, TEXT("0x"));
	_ltow(i, buf + 2, 0x10);
	//ltos?
	return buf;
}
*/
void FreeAndZero(void* mem)
{
	if (mem)
	{
		free(mem);
	//	mem = NULL;
	}
}

LPSTR StoreStringA(LPSTR str){
	if (!str)
		return NULL;

    LPSTR ret=(LPSTR)malloc(lstrlenA(str)+1);
    lstrcpyA(ret,str);
    return ret;
}
LPWSTR StoreStringW(LPWSTR str){
	if (!str)
		return NULL;

    LPWSTR ret=(LPWSTR)malloc(lstrlenW(str)*sizeof(WCHAR)+sizeof(WCHAR));
    lstrcpyW(ret,str);
	
    return ret;
}
LPSTR StoreNonNullStringA(LPSTR str){
	if (!str)
		return NULL;
	if (!str[0])
		return NULL;

    LPSTR ret=(LPSTR)malloc(lstrlenA(str)+1);
    lstrcpyA(ret,str);
    return ret;
}
LPWSTR StoreNonNullStringW(LPWSTR str){
	if (!str)
		return NULL;
	if (!str[0])
		return NULL;

    LPWSTR ret=(LPWSTR)malloc(lstrlenW(str) * sizeof(WCHAR) + sizeof(WCHAR));
    lstrcpyW(ret,str);
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
wchar_t* ConvertToLowercaseA(char* in, char* out)
{
	//while (*in)
		//так пока не пашет!	
		//*out++ = tolower(*in++);
	*out = '\0';
	return (wchar_t*)out;
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
			mem = (LPTSTR)malloc(size*sizeof(WCHAR) + sizeof(WCHAR));
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
	void __set (TCHAR* s)
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
