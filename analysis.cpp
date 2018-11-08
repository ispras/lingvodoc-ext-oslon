#define UNICODE

#ifdef __linux__ 
#include <wchar.h>
#include <string.h>
#include <wctype.h>
#include <stdlib.h>
#include "unicode.h"
#include "strings.h"
//typedef LPTSTR string;
#else
#include "default.h"
#include "windows.h"
#include "winnt.h"
#include <seh.h>
#endif

#include "stringfunctions.h"

#ifndef __linux__ 
#include "gui_win32.h"
#endif

#include "infotree.h"
#include "dictionary.h"
#include "cognates.h"

//летит
//TCHAR IPA[100] = TEXT("a, b");

#ifdef __linux__ 
extern "C" {int
#else
int __declspec(dllexport)
#endif
PhonemicAnalysis_GetAllOutput(LPTSTR bufIn, LPTSTR bufOut)
{
	//	try
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
		trOut.AddSubtree(&trIPANotFound, L"Неопознанные знаки", IT_COLUMN | IT_EMPTYLINEBEFORE, 0);

		InfoNode* ndDistr[FT_NSOUNDCLASSES];
		ndDistr[FT_VOWEL] = trOut.Add(L"Списки по гласному первого слога",/*NULL,*/ IT_COLUMN | IT_EMPTYLINEBEFORE);
		ndDistr[FT_CONSONANT] = trOut.Add(L"Списки по согласному перед гласным первого слога", /*NULL,*/ IT_COLUMN | IT_EMPTYLINEBEFORE);
		dic.BuildDistributionLists(ndDistr, &trOut);


		OutputString output; //Dictionary::OutputString катит!!!
		output.Build(trOut.ndRoot);

		lstrcpy(bufOut, output.bufOut);
		return 1;
	}
	//	catch (...)
	{
		return 2;
	}
}
#ifdef __linux__ 
}
#endif

#ifdef __linux__ 
extern "C" {int
#else
int __declspec(dllexport)
#endif
CognateAnalysis_GetAllOutput(LPTSTR bufIn, int nRows, int nCols, LPTSTR bufOut)
{
	if (nCols < 1 || nCols > 100)
		return 10;

	try
	{
		Comparison cmp(nRows, nCols);
		InfoTree trOut(L"ЭТИМОЛОГИЧЕСКИЙ АНАЛИЗ");

		cmp.AddCognateList(bufIn);

		Query qry;
		qry.AddCondition(L"Г", L"#", NULL, 0, L"Соответствия по начальному гласному");
		qry.AddCondition(L"С", L"#", NULL, 0, L"Соответствия по начальному согласному");
		qry.AddCondition(L"Г", L"С", NULL, QF_OBJECTONLYONCE, L"Соответствия по гласному после первого согласного");

		cmp.OutputLanguageNames(&trOut);

		for (Condition* cnd = qry.FirstCondition(); cnd; cnd = qry.NextCondition())
		{
			cmp.Process(cnd);
			cmp.Output(cnd, &trOut);
		}

		OutputString output;
		output.Build(trOut.ndRoot);

		lstrcpy(bufOut, output.bufOut);
		return 1;
	}
	catch (...)
	{
		return 2;
	}
}
#ifdef __linux__ 
}
#endif

#ifdef __linux__ 
extern "C" {int
#else
int __declspec(dllexport)
#endif
Retranscribe(LPTSTR bufIn, LPTSTR bufOut, LPTSTR langIn, LPTSTR langOut, int flags)
{
	try
	{
		Dictionary dic;

		dic.GuessReplacer(bufIn);

		dic.ReplaceSymbols(bufIn, bufOut);


		//lstrcpy(bufOut, bufIn);
		return 1;
	}
	catch (...)
	{
		return 2;
	}
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
