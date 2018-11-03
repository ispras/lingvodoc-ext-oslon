#define UNICODE

#ifdef __linux__ 
#include <wchar.h>
#include <string.h>
#include <wctype.h>
#include <stdlib.h>
#include "unicode.h"
//#include "strings.h"
//typedef LPTSTR string;
#else
#include "default.h"
#include "windows.h"
#include "winnt.h"
#endif

//#include "gui_win32.h"
//#include "strings.h"

#include "datastructures.h"
#include "infotree.h"
#include "parser.h"
#include "ipa.h"
#include "dictquery.h"
#include "dictionary.h"
#include "pool.h"
#include "btree.h"

//летит
//TCHAR IPA[100] = TEXT("a, b");

#ifdef __linux__ 
extern "C" {int
#else
int __declspec(dllexport)
#endif
GetAllOutput(LPTSTR bufIn, LPTSTR bufOut)
{
	InfoTree trOut(L"ФОНЕТИЧЕСКИЙ АНАЛИЗ");
	Dictionary dic;

	dic.AddWordList(bufIn);

	InfoTree trIPAVowels, trIPAConsonants, trIPANotFound;

	dic.BuildIPATable(FT_VOWEL, &trIPAVowels);
	dic.BuildIPATable(FT_CONSONANT, &trIPAConsonants);
	dic.BuildIPATable(FT_UNKNOWNSOUND, &trIPANotFound);

	trOut.AddSubtree(&trIPAConsonants, L"Согласные звуки", IT_COLUMN | IT_EMPTYLINEBEFORE | IT_LINEBRKAFTER, IT_TAB);
	trOut.AddSubtree(&trIPAVowels, L"Гласные звуки", IT_COLUMN | IT_EMPTYLINEBEFORE | IT_LINEBRKAFTER, IT_TAB);
	trOut.AddSubtree(&trIPANotFound, L"Неопознанные знаки", IT_COLUMN | IT_EMPTYLINEBEFORE, IT_COMMA | IT_SPACE);

	InfoNode* ndDistr[FT_NSOUNDCLASSES];
	ndDistr[FT_VOWEL] = trOut.Add(L"Списки по гласному первого слога", NULL, IT_COLUMN | IT_EMPTYLINEBEFORE);
	ndDistr[FT_CONSONANT] = trOut.Add(L"Списки по согласному перед гласным первого слога", NULL, IT_COLUMN | IT_EMPTYLINEBEFORE);
	dic.BuildDistributionLists(ndDistr, &trOut);


	OutputString output; //Dictionary::OutputString катит!!!
	output.Build(trOut.ndRoot);

	lstrcpy(bufOut, output.bufOut);

	return 1;
}
#ifdef __linux__ 
}
#endif

/*
BOOL WINAPI DllMain(_In_ HINSTANCE hinstDLL, _In_ DWORD fdwReason, _In_ LPVOID lpvReserved)
{
//	MsgBox(ipaVowels);
}
*/
