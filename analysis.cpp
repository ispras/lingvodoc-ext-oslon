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
//#include <wchar.h> пока не выходит
#include "windows.h"
#include "winnt.h"
#include <seh.h>
#include "gui_win32.h"
#endif

#include "stringfunctions.h"

#include "infotree.h"
#include "dictionary.h"
#include "distances.h"
#include "cognates.h"

//летит
//TCHAR IPA[100] = TEXT("a, b");

#ifdef __linux__ 
extern "C" {int
#else
int __declspec(dllexport)
#endif
PhonemicAnalysis_GetAllOutput(LPTSTR bufIn, int nRows, LPTSTR bufOut, int)
{
	int szOutput;

	if (nRows == -1)
		szOutput = lstrlen(bufIn) * 50 + 100000;
	else
		szOutput = nRows * 250 + 100000;

	if (!bufIn)
		return szOutput;

	try
	{
		InfoTree trOut(L"ФОНЕТИЧЕСКИЙ АНАЛИЗ");
		Dictionary dic;

		dic.AddWordList(bufIn, nRows - 1);

		InfoTree trIPAVowels, trIPAConsonants, trIPANotFound;

		dic.BuildIPATable(FT_VOWEL, &trIPAVowels);
		dic.BuildIPATable(FT_CONSONANT, &trIPAConsonants);
		dic.BuildIPATable(FT_UNKNOWNSOUND, &trIPANotFound);

		trOut.AddSubtree(&trIPAConsonants, L"Согласные звуки", IT_COLUMN | IT_EMPTYLINEBEFORE);//, IT_TAB);
		trOut.AddSubtree(&trIPAVowels, L"Гласные звуки", IT_COLUMN | IT_EMPTYLINEBEFORE);//, IT_TAB);
		trOut.AddSubtree(&trIPANotFound, L"Неопознанные знаки", IT_COLUMN | IT_EMPTYLINEBEFORE, 0);

		InfoNode* ndDistr[FT_NSOUNDCLASSES];
		ndDistr[FT_VOWEL] = trOut.Add(L"Списки по гласному первого слога",/*NULL,*/ IT_COLUMN | IT_EMPTYLINEBEFORE);
		ndDistr[FT_CONSONANT] = trOut.Add(L"Списки по согласному перед гласным первого слога", /*NULL,*/ IT_COLUMN | IT_EMPTYLINEBEFORE);
		dic.BuildDistributionLists(ndDistr, &trOut);

		OutputString output(szOutput, 100); //Dictionary::OutputString катит!!!
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
CognateAnalysis_GetAllOutput(LPTSTR bufIn, int nCols, int nRows, LPTSTR bufOut, int flags)
{
	if (nCols < 1 || nCols > 1000)
		return -1;

	int szOutput = nRows * nCols * 60 + 100000;

	if (!bufIn)
		return szOutput;

	try
	{
		bool isBinary = flags == 2;
		nRows -= 1;// потому что там ещё и заголовок

		Comparison cmp(nRows, nCols);
		LPTSTR title;
		if (!isBinary) title = L"ЭТИМОЛОГИЧЕСКИЙ АНАЛИЗ"; else title = NULL;
		InfoTree trOut(title);

		cmp.AddCognateList(bufIn, false);

		Query qry;
		qry.AddCondition(L"Г", L"#", NULL, 0, L"Соответствия по начальному гласному");
		qry.AddCondition(L"Г", L"С", NULL, QF_OBJECTONLYONCE, L"Соответствия по гласному после первого согласного");
		qry.AddCondition(L"С", L"#", NULL, 0, L"Соответствия по начальному согласному");

		if (!isBinary)
			cmp.OutputLanguageList(&trOut);

		for (Condition* cnd = qry.FirstCondition(); cnd; cnd = qry.NextCondition())
		{
			cmp.Process(cnd);
			cmp.OutputCorresponcesWithMaterial(cnd, &trOut);
		}

		OutputString output(szOutput, 20, nCols * 2, isBinary);
		output.Build(trOut.ndRoot);

		if (isBinary)
			output.OutputTableSizes(bufOut);
		output.OutputData(bufOut);

		return output.OutputSize;
	}
	catch (...)
	{
		return -2;
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
CognateDistanceAnalysis_GetAllOutput(LPTSTR bufIn, int nCols, int nRows, LPTSTR bufOut, int flags)
{
	if (nCols < 1 || nCols > 1000)
		return -1;

	int szOutput = nRows * nCols * 60 + 300;

	if (!bufIn)
		return szOutput;

	try
	{
		bool isBinary = flags == 2;
		nRows -= 1;// потому что там ещё и заголовок

		Comparison cmp(nRows, nCols);
		LPTSTR title;
		if (!isBinary) title = L"ВЫЧИСЛЕНИЕ ПОПАРНЫХ РАССТОЯНИЙ"; else title = NULL;
		InfoTree trOut(title);

		cmp.AddCognateList(bufIn, false);

		Query qry;

		qry.AddCondition(L"Г", NULL, NULL, QF_OBJECTONLYONCE, L"Соответствия по первому гласному");
		//		qry.AddCondition(L"Г", L"#", NULL, 0, 					L"Соответствия по начальному гласному");
		//		qry.AddCondition(L"Г", L"С", NULL, QF_OBJECTONLYONCE, 	L"Соответствия по гласному после первого согласного");
		qry.AddCondition(L"С", L"#", NULL, 0, L"Соответствия по начальному согласному");

		if (!isBinary)
			cmp.OutputLanguageList(&trOut);


		DistanceMatrix mtxSum(cmp.nDicts);

		for (Condition* cnd = qry.FirstCondition(); cnd; cnd = qry.NextCondition())
		{
			cmp.Process(cnd);

			DistanceMatrix mtx(cmp.nDicts);


			cmp.CalculateDistances(&mtxSum);

			cmp.CalculateDistances(&mtx);
			cmp.OutputDistances(cnd, &mtx, &trOut);
		}


		Condition* cnd = qry.AddCondition(NULL, NULL, NULL, 0, L"Суммарная матрица");
		cmp.OutputDistances(cnd, &mtxSum, &trOut);


		OutputString output(szOutput, 20, nCols * 2, isBinary);
		output.Build(trOut.ndRoot);

		if (isBinary)
			output.OutputTableSizes(bufOut);
		output.OutputData(bufOut);

		return output.OutputSize;
	}
	catch (...)
	{
		return -2;
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
CognateAcousticAnalysis_GetAllOutput(LPTSTR bufIn, int nCols, int nRows, LPTSTR bufOut, int flags)
{
	if (nCols < 1 || nCols > 1000)
		return -1;

	int szOutput = nRows * nCols * 1000 + 100000;

	if (!bufIn)
		return szOutput;

	try
	{
		bool isBinary = flags == 2;
		nRows -= 1;// потому что там ещё и заголовок

		Comparison cmp(nRows, nCols);
		LPTSTR title;
		if (!isBinary) title = L"АКУСТИЧЕСКИЙ АНАЛИЗ ОТКЛОНЕНИЙ"; else title = NULL;
		InfoTree trOut(title);
		InfoTree trCld(NULL);

		cmp.AddCognateList(bufIn, true);

		Query qry;
		//qry.AddCondition(L"Г", L"#", NULL, 0, 					L"Отклонения по начальному гласному");
		//qry.AddCondition(L"Г", L"С", NULL, QF_OBJECTONLYONCE, 	L"Отклонения по гласному после первого согласного");

		qry.AddCondition(L"Г", NULL, NULL, QF_OBJECTONLYONCE, L"Отклонения по первому гласному в слове");

		if (!isBinary)
			cmp.OutputLanguageList(&trOut);

		for (Condition* cnd = qry.FirstCondition(); cnd; cnd = qry.NextCondition())
		{
			cmp.Process(cnd);
			cmp.OutputDeviationsWithMaterial(cnd, &trOut, &trCld);
		}

		OutputString outputText(szOutput, 20, 7, isBinary); //7 столбцов, см. OutputPhoneticHeader
		OutputString outputClouds(szOutput, 20, 4, isBinary); //4 столбца, токмо дѣля цыфирієвъ

		outputText.Build(trOut.ndRoot);
		outputClouds.Build(trCld.ndRoot);

		if (isBinary)
			outputText.OutputTableSizes(bufOut);

		outputText.OutputData(bufOut);
		bufOut[outputText.OutputSize] = L'\0';

		LPTSTR posOutClouds = bufOut + outputText.OutputSize + 1 * isBinary;

		if (isBinary)
		{
			bufOut[outputText.OutputSize + 1] = L'\0';
			outputClouds.OutputTableSizes(posOutClouds);
		}

		outputClouds.OutputData(posOutClouds);

		return outputText.OutputSize + 1 + outputClouds.OutputSize;
	}
	catch (...)
	{
		return -2;
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

		dic.ReplaceSymbols(bufIn, bufOut/*, 200*/, dic.GuessReplacer(bufIn));


		//lstrcpy(bufOut, bufIn);
		return 1;
	}
	catch (...)
	{
		return -2;
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
GetPhonemeDifference(LPTSTR bufIn, LPTSTR bufOut)
{
	try
	{
		InfoTree trOut(L"МЕЖФОНЕМНОЕ РАССТОЯНИЕ");

		Dictionary dic;

		TCHAR bufIPA[1000];
		dic.ReplaceSymbols(bufIn, bufIPA, dic.GuessReplacer(bufIn));

		Parser parser(bufIPA, L" ");
		Sound* s[2];
		s[0] = s[1] = NULL;

		dic.ipa->SubmitWordForm(bufIPA);
		dic.ipa->EndSubmitWordForms();

		for (int i = 0; parser.Next() && i <= 1; i++)
		{
			Segmentizer sgmntzr(dic.ipa, parser.Current());

			s[i] = sgmntzr.GetNext();
		}

		DistanceMatrix mtx(2);

		InfoNode* in;
		TCHAR buf[1000];

		trOut.Add(NULL, IT_HORLINE);

		for (int i = 0; i <= 1; i++)
		{
			if (s[i])
			{
				in = trOut.Add(s[i]->Symbol, IT_SQRBRK | IT_LINEBRKAFTER);

				for (int iFType = 0; iFType < FT_NFEATURETYPES; iFType++)
				{
					trOut.Add(dic.ipa->nameFeature[iFType], IT_COLUMN | IT_TAB);
					trOut.Add(strcpyh(buf, s[i]->feature[iFType], 8), IT_COLUMN | IT_TAB);

					dic.ipa->GetFeatureNames(s[i]->feature, iFType, buf);
					trOut.Add(buf, IT_LINEBRKAFTER);
				}

				//trOut.Add(NULL, IT_LINEBRK);
				trOut.Add(NULL, IT_HORLINE);
			}
		}

		for (int i = 0; i <= 1; i++)
		{
			if (s[i])
				trOut.Add(s[i]->Symbol, 0);
			else
				trOut.Add(L"?", 0);
			if (i == 0)
				trOut.Add(L" : ", 0);
		}

		trOut.Add(L" → ", 0);
		trOut.Add(strcpyi(bufOut, mtx.GetDistance(0, 1, s[0], s[1])), 0);

		OutputString output(2000, 100); //Dictionary::OutputString катит!!!
		output.Build(trOut.ndRoot);

		lstrcpy(bufOut, output.bufOut);
		return 1;
	}
	catch (...)
	{
		return -2;
	}
}
#ifdef __linux__ 
}
#endif

/*
BOOL WINAPI DllMain(_In_ HINSTANCE hinstDLL, _In_ DWORD fdwReason, _In_ LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	MsgBox(L"Порядочек-с!");
}
*/