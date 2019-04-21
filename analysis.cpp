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
#include "comparanda.h"
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
		qry.AddCondition(L"Г", L"#", NULL, QF_ITERATE, L"Соответствия по начальному гласному");
		qry.AddCondition(L"Г", L"С", NULL, QF_OBJECTONLYONCE | QF_ITERATE, L"Соответствия по гласному после первого согласного");
		qry.AddCondition(L"С", L"#", NULL, QF_ITERATE, L"Соответствия по начальному согласному");

		if (!isBinary)
			cmp.OutputLanguageList(&trOut);

		for (Condition* cnd = qry.FirstCondition(); cnd; cnd = qry.NextCondition())
		{
			cmp.Process(cnd, true);
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

		qry.AddCondition(L"Г", L"#", NULL, QF_ITERATE, L"Соответствия по начальному гласному (вес: 1)", 1);
		qry.AddCondition(L"Г", L"С", NULL, QF_OBJECTONLYONCE | QF_ITERATE, L"Соответствия по гласному после первого согласного (вес: 1)", 1);
		qry.AddCondition(L"С", L"#", NULL, QF_ITERATE, L"Соответствия по начальному согласному (вес: 5)", 5);

		if (!isBinary)
			cmp.OutputLanguageList(&trOut);


		DistanceMatrix mtxSum(cmp.dic.ipa, cmp.nDicts);

		for (Condition* cnd = qry.FirstCondition(); cnd; cnd = qry.NextCondition())
		{
			cmp.Process(cnd, true);

			DistanceMatrix mtx(cmp.dic.ipa, cmp.nDicts);


			cmp.CalculateDistances(&mtxSum, cnd->intExtra);

			cmp.CalculateDistances(&mtx, cnd->intExtra);

			cmp.OutputDistances(cnd, &mtx, &trOut);
		}


		Condition* cnd = qry.AddCondition(NULL, NULL, NULL, 0, L"Суммарная матрица");
		cmp.RemoveDistancesIfTooFew(&mtxSum, 20);
		cmp.OutputDistances(cnd, &mtxSum, &trOut);

		if (!isBinary)
		{
			cmp.SoundCorrespondenceNumbers(&trOut);
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

		qry.AddCondition(L"Г", NULL, NULL, QF_OBJECTONLYONCE | QF_ITERATE, L"Отклонения по первому гласному в слове");

		if (!isBinary)
			cmp.OutputLanguageList(&trOut);

		for (Condition* cnd = qry.FirstCondition(); cnd; cnd = qry.NextCondition())
		{
			cmp.Process(cnd, true);
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
	Dictionary dic;

	dic.ReplaceSymbols(bufIn, bufOut/*, 200*/, dic.GuessReplacer(bufIn));


	//lstrcpy(bufOut, bufIn);
	return 1;
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
	InfoTree trOut(L"МЕЖФОНЕМНОЕ РАССТОЯНИЕ");
	Dictionary dic;
	Comparandum cmp[2];
	TCHAR bufIPA[1000];
	DistanceMatrix mtx(dic.ipa, 2);
	InfoNode* in;
	TCHAR buf[1000];

	dic.ReplaceSymbols(bufIn, bufIPA, dic.GuessReplacer(bufIn));
	dic.ipa->SubmitWordForm(bufIPA);
	dic.ipa->EndSubmitWordForms();

	Parser parser(bufIPA, L" ");
	for (int i = 0; parser.Next() && i <= 1; i++)
		cmp[i].SetFragment(parser.Current());

	trOut.Add(NULL, IT_HORLINE);

	for (int i = 0; i <= 1; i++)
	{
		Segmentizer sgmntzr(dic.ipa, cmp[i].Text());

		Sound* s;
		while (s = sgmntzr.GetNext())
		{
			in = trOut.Add(s->Symbol, IT_SQRBRK | IT_LINEBRKAFTER);
			for (int iFType = 0; iFType < FT_NFEATURETYPES; iFType++)
			{
				trOut.Add(dic.ipa->nameFeature[iFType], IT_COLUMN | IT_TAB);
				trOut.Add(strcpyh(buf, s->feature[iFType], 8), IT_COLUMN | IT_TAB);

				dic.ipa->GetFeatureNames(s->feature, iFType, buf);
				trOut.Add(buf, IT_LINEBRKAFTER);
			}
		}
		trOut.Add(NULL, IT_HORLINE);
	}

	for (int i = 0; i <= 1; i++)
	{
		trOut.Add(cmp[i].Text(), IT_SQRBRK);
		if (i == 0)
			trOut.Add(L" : ", 0);
	}

	trOut.Add(L" → ", 0);
	trOut.Add(strcpyi(bufOut, mtx.GetDistance(0, 1, &cmp[0], &cmp[1], 1)), 0);

	OutputString output(2000, 100); //Dictionary::OutputString катит!!!
	output.Build(trOut.ndRoot);

	lstrcpy(bufOut, output.bufOut);
	return 1;
}
#ifdef __linux__ 
}
#endif


#ifdef __linux__ 
extern "C" {int
#else
int __declspec(dllexport)
#endif
ExtractCognateRows(LPTSTR bufIn, LPTSTR bufOut)
{
	Dictionary dic;

	Comparison cmp(0, 0);
	InfoTree trOut(L"ЕДИНИЧНЫЙ РЯД");

	cmp.AddCognateListText(bufIn);

	Query qry;
	qry.AddCondition(L"Г", L"#", NULL, QF_ITERATE, L"Соответствия по начальному гласному");
	qry.AddCondition(L"Г", L"С", NULL, QF_ITERATE | QF_OBJECTONLYONCE, L"Соответствия по гласному после первого согласного");
	qry.AddCondition(L"С", L"#", NULL, QF_ITERATE, L"Соответствия по начальному согласному");

	for (Condition* cnd = qry.FirstCondition(); cnd; cnd = qry.NextCondition())
	{
		cmp.Process(cnd, false);
		cmp.OutputCorresponcesWithMaterial(cnd, &trOut, true);
	}

	OutputString output(20000, 20, cmp.nDicts * 2, false);
	output.Build(trOut.ndRoot);

	output.OutputData(bufOut);
	return 1;
}

/*
BOOL WINAPI DllMain(_In_ HINSTANCE hinstDLL, _In_ DWORD fdwReason, _In_ LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	MsgBox(L"Порядочек-с!");
}
*/