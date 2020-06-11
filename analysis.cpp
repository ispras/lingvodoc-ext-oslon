//#define DEBUGMEM

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
#include "strings.h"
#include <seh.h>


#ifdef DEBUGMEM
#include "отладка памяти.h"
#endif

#include "gui_win32.h"

#endif



#include "stringfunctions.h"


#ifdef DEBUGMEM
#include "вывод отладки памяти.h"
#endif



//#include "отладка.cpp"



#include "infotree.h"
#include "dictionary.h"
#include "comparanda.h"
#include "distances.h"
#include "comparison.h"
#include "comparisonwithdistances.h"
#include "comparisonwithacoustics.h"
#include "comparisonwithguess.h"
#include "reconstruction.h"
#include "multireconstruction.h"

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
		szOutput = nRows * 350 + 100000;

	if (!bufIn)
		return szOutput;
#ifdef DEBUGMEM
	MemDbg _m;
#endif
	try
	{
		InfoTree trOut(L"ФОНЕТИЧЕСКИЙ АНАЛИЗ");
		Dictionary dic;

		dic.AddWordList(bufIn, nRows - 1);

		InfoTree trIPAVowels, trIPAConsonants, trIPANotFound;

		dic.BuildIPATable(FT_VOWEL, &trIPAVowels);
		dic.BuildIPATable(FT_CONSONANT, &trIPAConsonants);
		dic.BuildIPATable(FT_UNKNOWNSOUND, &trIPANotFound);

		trOut.HorLine();
		trOut.Add(dic.dictinfo.name, 0);
		//trOut.HorLine();

		trOut.AddSubtree(&trIPAConsonants, L"Согласные звуки", IT_COLUMN | IT_EMPTYLINEBEFORE);//, IT_TAB);
		trOut.AddSubtree(&trIPAVowels, L"Гласные звуки", IT_COLUMN | IT_EMPTYLINEBEFORE);//, IT_TAB);
		trOut.AddSubtree(&trIPANotFound, L"Неопознанные знаки", IT_COLUMN | IT_EMPTYLINEBEFORE, 0);

		InfoNode* ndTblDistr[FT_NSOUNDCLASSES];
		ndTblDistr[FT_VOWEL] = trOut.Add(L"Распределение гласных", IT_COLUMN | IT_EMPTYLINEBEFORE | IT_LINEBRKAFTER);
		ndTblDistr[FT_CONSONANT] = trOut.Add(L"Распределение согласных", IT_COLUMN | IT_EMPTYLINEBEFORE);

		InfoNode* ndListDistr[FT_NSOUNDCLASSES];
		ndListDistr[FT_VOWEL] = trOut.Add(L"Списки по гласному первого слога",/*NULL,*/ IT_COLUMN);
		trOut.HorLine();
		ndListDistr[FT_CONSONANT] = trOut.Add(L"Списки по согласному перед гласным первого слога", /*NULL,*/ IT_COLUMN);

		dic.phono->AddDistibutionConditions();

		dic.BuildDistributionTables(dic.phono->qry, ndTblDistr, &trOut);
		dic.BuildDistributionLists(dic.phono->qry, ndListDistr, &trOut);

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
GuessCognates_GetAllOutput(LPTSTR bufIn, int nCols, int nRowsCorresp, int nRowsRest, int iDictThis, int lookMeaning, int onlyOrphans, LPTSTR bufOut, int flags)
{
	if (nCols < 1 || nCols > 1000)
		return -1;

	int nRows = nRowsCorresp + nRowsRest;
	int szOutput = nRows * nCols * 2000 + 100000;

	if (!bufIn)
		return szOutput;
#ifdef DEBUGMEM
	MemDbg _m;
#endif
	try
	{
		bool isBinary = flags == 2;
		nRowsCorresp -= 1;// потому что там ещё и заголовок

		ComparisonWithGuess cmp(nRowsCorresp, nCols, nRowsRest);
		//		InfoTree trOut(isBinary ? NULL : L"АВТОСООТВЕТСТВИЯ");//gcc не хочет!
		LPTSTR title;
		if (!isBinary) title = L"АВТОСООТВЕТСТВИЯ"; else title = NULL;
		InfoTree trOut(title);

		cmp.Input(bufIn);

		if (!isBinary)
			cmp.OutputLanguageList(&trOut);


		Query qry;
		const int nCnd = 2;
		Condition* cnd[nCnd];

		cnd[0] = qry.AddCondition(L"С", L"#", NULL, QF_ITERATE | QF_OBJECTONLYONCE, L"Соответствия по начальному согласному");
		cnd[1] = qry.AddCondition(L"С", L"Г", NULL, QF_ITERATE | QF_OBJECTONLYONCE, L"Соответствия по согласному после первого гласного", 0, 1);

		cmp.ProcessAndOutput(&trOut, cnd, nCnd, iDictThis, lookMeaning, onlyOrphans);

		return trOut.Output(bufOut, szOutput, 20, nCols * 2, isBinary);
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
CognateAnalysis_GetAllOutput(LPTSTR bufIn, int nCols, int nRows, LPTSTR bufOut, int flags)
{
	if (nCols < 1 || nCols > 1000)
		return -1;

	int szOutput = nRows * nCols * 150 + 100000;

	if (!bufIn)
		return szOutput;
#ifdef DEBUGMEM
	MemDbg _m;
#endif
	try
	{
		bool isBinary = flags == 2;
		nRows -= 1;// потому что там ещё и заголовок

		Comparison cmp(nRows, nCols);
		LPTSTR title;
		if (!isBinary) title = L"ЭТИМОЛОГИЧЕСКИЙ АНАЛИЗ"; else title = NULL;
		InfoTree trOut(title);

		cmp.Input(bufIn, false);

		Query qry;
		//		qry.AddCondition(L"Г", L"#", NULL, QF_ITERATE,						L"Соответствия по начальному гласному");
		qry.AddCondition(L"(С", L"#", NULL, QF_ITERATE, L"Соответствия по начальному согласному");
		//		qry.AddCondition(L"Г", L"(С", NULL, QF_ITERATE|QF_DELETENULLPREV,	L"Соответствия по гласному первого слога (после согласного)", 0, 1);
		//		qry.AddCondition(L"С", L"Г", NULL, QF_ITERATE,						L"Соответствия по согласному после гласного первого слога", 0, 1);
		//		qry.AddCondition(L"Г", L"С", NULL, QF_ITERATE,						L"Соответствия по гласному второго слога", 0, 2);

		if (!isBinary)
			cmp.OutputLanguageList(&trOut);

		for (Condition* cnd = qry.FirstCondition(); cnd; cnd = qry.NextCondition())
		{
			//cmp.Process(cnd, false, false);
			//cmp.Process(cnd, false, true);
			//cmp.Process(cnd, true, false);
			cmp.Process(cnd, true, true);
			cmp.OutputCorrespondencesWithMaterial(cnd, &trOut);
		}

		return trOut.Output(bufOut, szOutput, 20, nCols * 2, isBinary);
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
#ifdef DEBUGMEM
	MemDbg _m;
#endif
	try
	{
		bool isBinary = flags == 2;
		nRows -= 1;// потому что там ещё и заголовок

		ComparisonWithDistances cmp(nRows, nCols);
		LPTSTR title;
		if (!isBinary) title = L"ВЫЧИСЛЕНИЕ ПОПАРНЫХ РАССТОЯНИЙ"; else title = NULL;
		InfoTree trOut(title);

		cmp.Input(bufIn, false);

		Query qry;

		qry.AddCondition(L"Г", L"#", NULL, QF_ITERATE, L"Соответствия по начальному гласному (вес: 1)", 1);
		qry.AddCondition(L"Г", L"(С", NULL, QF_OBJECTONLYONCE | QF_DELETENULLPREV | QF_ITERATE, L"Соответствия по гласному первого слога (после согласного) (вес: 1)", 1);
		qry.AddCondition(L"(С", L"#", NULL, QF_ITERATE, L"Соответствия по начальному согласному (вес: 1)", 1);

		if (!isBinary)
			cmp.OutputLanguageList(&trOut);


		DistanceMatrix mtxSum(cmp.llDicts.first->ipa, cmp.nDicts);

		for (Condition* cnd = qry.FirstCondition(); cnd; cnd = qry.NextCondition())
		{
			cmp.Process(cnd, true, true);

			DistanceMatrix mtx(cmp.llDicts.first->ipa, cmp.nDicts);

			cmp.CalculateDistances(&mtxSum, cnd->intExtra);

			cmp.CalculateDistances(&mtx, cnd->intExtra);

			cmp.OutputDistances(cnd, &mtx, &trOut);
		}


		int threshold = 10;

		Condition* cnd = qry.AddCondition(NULL, NULL, NULL, 0, L"Суммарная матрица");
		cmp.RemoveDistancesIfTooFew(&mtxSum, threshold);
		cmp.OutputDistances(cnd, &mtxSum, &trOut);

		if (!isBinary)
		{
			cmp.SoundCorrespondenceNumbers(&trOut, threshold);
		}


		return trOut.Output(bufOut, szOutput, 20, nCols * 2, isBinary);
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
#ifdef DEBUGMEM
	MemDbg _m;
#endif
	try
	{
		bool isBinary = flags == 2;
		nRows -= 1;// потому что там ещё и заголовок

		Comparison cmp(nRows, nCols);
		LPTSTR title;
		if (!isBinary) title = L"АКУСТИЧЕСКИЙ АНАЛИЗ ОТКЛОНЕНИЙ"; else title = NULL;
		InfoTree trOut(title);
		InfoTree trCld(NULL);

		cmp.Input(bufIn, true);

		Query qry;
		//qry.AddCondition(L"Г", L"#", NULL, 0,                     L"Отклонения по начальному гласному");
		//qry.AddCondition(L"Г", L"С", NULL, QF_OBJECTONLYONCE,     L"Отклонения по гласному после первого согласного");

		qry.AddCondition(L"Г", NULL, NULL, QF_OBJECTONLYONCE | QF_ITERATE, L"Отклонения по первому гласному в слове");

		if (!isBinary)
			cmp.OutputLanguageList(&trOut);

		for (Condition* cnd = qry.FirstCondition(); cnd; cnd = qry.NextCondition())
		{
			cmp.Process(cnd, true, true);
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
CognateReconstruct_GetAllOutput(LPTSTR bufIn, int nCols, int nRows, LPTSTR bufOut, int flags)
{
	if (nCols < 1 || nCols > 1000)
		return -1;

	int szOutput = nRows * nCols * 160 + 100000;

	if (!bufIn)
		return szOutput;
#ifdef DEBUGMEM
	MemDbg _m;
#endif
	try
	{
		bool isBinary = flags == 2;

		LPTSTR title;
		if (!isBinary) title = L"РЕКОНСТРУКЦИЯ ПРАФОРМ"; else title = NULL;
		InfoTree trOut(title);

		Reconstruction rc(nCols, nRows - 1, bufIn);//nRows -= 1;//потому что там ещё и заголовок

		nCols = rc.Reconstruct();


		if (!isBinary)
			rc.comparisons[0].OutputLanguageList(&trOut);
		for (int i = 0; i < rc.nComparisons; i++)
			rc.comparisons[i].OutputReconstructedSounds(rc.comparisons[i].condition, &trOut);
		trOut.Add(NULL, IT_SECTIONBRK, NULL);
		rc.comparisons[0].OutputReconstructedWords(&trOut);

		return trOut.Output(bufOut, szOutput, 20, nCols * 2, isBinary);
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
CognateMultiReconstruct_GetAllOutput(LPTSTR bufIn, int* pnCols, int nGroups, int nRows, LPTSTR bufOut, int flags)
{
	int nColsAll = 0;
	for (int i = 0; i < nGroups; i++)
		nColsAll += pnCols[i];
	if (nColsAll < 1 || nColsAll > 1000)
		return -1;
	int szOutput = nRows * nColsAll * 160 + 100000;

	if (!bufIn)
		return szOutput;

	try
	{
#ifdef DEBUGMEM
		MemDbg _m;
#endif
		bool isBinary = flags == 2;

		LPTSTR title;
		if (!isBinary) title = L"РЕКОНСТРУКЦИЯ ПРАПРАФОРМ"; else title = NULL;
		InfoTree trOut(title);

		MultiReconstruction mrc(pnCols, nGroups, nRows - 1, bufIn);

		mrc.ReconstructFirstLevel();

		Reconstruction rc(nGroups, nRows - 1, NULL);//nRows - 1, потому что там ещё и заголовок

		for (int i = 0; i < nGroups; i++)
			rc.CopyColumnFrom(mrc.reconstructions[i], 0, i);

		int nCols = rc.Reconstruct();

		if (!isBinary)
			rc.comparisons[0].OutputLanguageList(&trOut);
		for (int i = 0; i < rc.nComparisons; i++)
			rc.comparisons[i].OutputReconstructedSounds(rc.comparisons[i].condition, &trOut);

		trOut.Add(NULL, IT_SECTIONBRK, NULL);
		rc.comparisons[0].OutputReconstructedWords(&trOut);

		return trOut.Output(bufOut, szOutput, 20, nCols * 2, isBinary);
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
#ifdef DEBUGMEM
	MemDbg _m;
#endif
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
#ifdef DEBUGMEM
	MemDbg _m;
#endif
	InfoTree trOut(L"МЕЖФОНЕМНОЕ РАССТОЯНИЕ");
	Dictionary dic;
	Comparandum cmp[2];
	TCHAR bufIPA[1000];
	DistanceMatrix mtx(dic.ipa, 2);
	InfoNode* in;
	TCHAR buf[1000];

	dic.ReplaceSymbols(bufIn, bufIPA, dic.GuessReplacer(bufIn));
	//dic.ipa->ConfirmAllIPA();
	//dic.ipa->SubmitWordForm(bufIPA);
	//dic.ipa->EndSubmitWordForms();

	Parser parser(bufIPA, L" ");
	for (int i = 0; parser.Next() && i <= 1; i++)
	{
		if (parser.Current()[0] == L'@')
			cmp[i].sg.typeOfSegment = ST_NULL;
		else
		{
			cmp[i].sg.SetFragment(parser.Current());
			cmp[i].isSoundInCognates = true;
		}
	}

	trOut.Add(NULL, IT_HORLINE);

	for (int i = 0; i <= 1; i++)
	{
		if (cmp[i].sg.typeOfSegment != ST_NULL)
		{
			Segmentizer sgmntzr(dic.ipa, cmp[i].sg.Text());

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
		}
		else
		{
			in = trOut.Add(cmp[i].sg.Text(), IT_LINEBRKAFTER);
		}
		trOut.Add(NULL, IT_HORLINE);
	}

	for (int i = 0; i <= 1; i++)
	{
		trOut.Add(cmp[i].sg.Text(), IT_SQRBRK);
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
#ifdef DEBUGMEM
	MemDbg _m;
#endif
	Comparison cmp(0, 0);
	InfoTree trOut(L"ЕДИНИЧНЫЙ РЯД");

	cmp.AddCognateListText(bufIn);
	cmp.nRowsCorresp = 1;

	Query qry;
	qry.AddCondition(L"Г", L"#", NULL, QF_ITERATE, L"Соответствия по начальному гласному");
	qry.AddCondition(L"(С", L"#", NULL, QF_ITERATE, L"Соответствия по начальному согласному");
	qry.AddCondition(L"Г", L"(С", NULL, QF_ITERATE | QF_DELETENULLPREV, L"Соответствия по гласному первого слога (после согласного)", 0, 1);
	qry.AddCondition(L"С", L"Г", NULL, QF_ITERATE, L"Соответствия по согласному после гласного первого слога", 0, 1);
	qry.AddCondition(L"Г", L"С", NULL, QF_ITERATE, L"Соответствия по гласному второго слога", 0, 2);

	for (Condition* cnd = qry.FirstCondition(); cnd; cnd = qry.NextCondition())
	{
		cmp.Process(cnd, false, false);
		cmp.OutputCorrespondencesWithMaterial(cnd, &trOut, true);
	}

	OutputString output(20000, 20, cmp.nDicts * 2, false);
	output.Build(trOut.ndRoot);

	output.OutputData(bufOut);
	return 1;
}
#ifdef __linux__ 
}
#endif


/*
BOOL WINAPI DllMain(_In_ HINSTANCE hinstDLL, _In_ DWORD fdwReason, _In_ LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		MsgBox(L"вошли");
		break;
	case DLL_PROCESS_DETACH:
		MsgBox(L"вышли");
		break;
	}
	return true;
}
*/