#pragma once
#include "datastructures.h"
#include "parser.h"
#include "ipa.h"

#ifndef __linux__ 
void __declspec(dllexport) _initcalc(void*);
void __declspec(dllexport) _addcalc(void*, LPTSTR);
LPTSTR __declspec(dllexport) _donecalc(void*, LPTSTR, int);

//void __declspec(dllexport) _initcalc();
//void __declspec(dllexport) _addcalc(LPTSTR);
//LPTSTR __declspec(dllexport) _donecalc(LPTSTR);
#endif

class CognateList;

class Comparison
{
public:
	Dictionary 			dic;
	//Pool<TCHAR>		pString;

	int			nDicts;
	int 		nCorresp;
	int			nSoundCorresp;
	bool		isProcessed;
	DictInfo*	dictinfos;


	Correspondence*		corresps;
	CorrespondenceTree 	tCorrespondences;

	Comparison(int nRows, int nCols) : tCorrespondences(nCols)
	{
		nDicts = nCols;
		nCorresp = nRows;
		nSoundCorresp = 0;

		AllocDictData();

		isProcessed = false;
	}
	void AllocDictData()
	{
		//corresps = new Correspondence[nRows];
		corresps = (Correspondence*)malloc(nCorresp * sizeof(Correspondence));

		dictinfos = (DictInfo*)malloc(nDicts * sizeof(DictInfo));//new DictInfo[nDicts];
		for (int i = 0; i < nDicts; i++) new (&dictinfos[i]) DictInfo(L"?");
	}

	~Comparison()
	{
		for (int iRow = 0; iRow < nCorresp; iRow++)
		{
			if (corresps[iRow].comparanda)
			{
				free(corresps[iRow].comparanda);//corresps[iRow].~Correspondence();
				corresps[iRow].comparanda = NULL;
			}
		}

		//delete[] corresps; и т.п.
		free(corresps);
		free(dictinfos);
	}
	void Reset()
	{
		if (isProcessed)
		{
			tCorrespondences.Empty();

			for (int iRow = 0; iRow < nCorresp; iRow++)
			{
				corresps[iRow].crspNextSame = NULL;
				for (int iCol = 0; iCol < nDicts; iCol++)
					corresps[iRow].comparanda[iCol].Reset_();//почему-то Reset — неоднозначно

				corresps[iRow].rankAllSoundsSame = 0;
				corresps[iRow].nSoundsSame = 0;
				corresps[iRow].nSoundsEmpty = 0;
				corresps[iRow].degenerate = false;

				corresps[iRow].dataExtra = NULL;
			}
			isProcessed = false;
		}
	}
	void AddCognateList(LPTSTR sIn, bool hasPhonData)
	{
		Parser parser(sIn, L"\0", PARSER_NONNULLEND);
		LPTSTR wordOrig = NULL, wordIPA = NULL, wordTranslation = NULL, wchrTranscr = NULL, wLength = NULL, wF1 = NULL, wF2 = NULL, wF3 = NULL;

		int iRow = -1, iCol = -1;
		while (parser.Next())
		{
			if (!dic.NextCol(iCol, iRow, nDicts, nCorresp)) break;

			if (iRow == -1)
				dic.GetDictInfo(parser, dictinfos[iCol]);
			else
			{
				dic.GetOrigIPAAndTranslation(parser, wordOrig, wordIPA, wordTranslation, dictinfos[iCol], hasPhonData, wchrTranscr, wLength, wF1, wF2, wF3);
				if (iCol == 0)
					new (&corresps[iRow]) Correspondence(nDicts, iRow);

				new (&corresps[iRow].comparanda[iCol]) Comparandum(wordIPA, wordOrig, wordTranslation, wchrTranscr, wLength, wF1, wF2, wF3);

				if (wordIPA)
					dictinfos[iCol].nWords++;
			}
		}
		dic.ipa->EndSubmitWordForms();
	}
	void AddCognateListText(LPTSTR sIn)
	{
		LPTSTR _sin = dic.StoreString(sIn);/*это просто копия*/

		Parser parser(_sin, L"\t");
		LPTSTR wordOrig, wordIPA;

		nCorresp = 1; //пока только одна строчка
		while (parser.Next()) nDicts++;

		AllocDictData();

		parser.ResetText(sIn);

		int iRow = -1, iCol = -1;
		while (parser.Next())
		{

			iCol++;
			wordOrig = dic.StoreString(parser.Current(), parser.LengthOfCurrentWord());
			wordIPA = dic.TranscribeWord(wordOrig, dictinfos[iCol]);

			if (iCol == 0)
			{
				iRow++;
				new (&corresps[iRow]) Correspondence(nDicts, iRow);
			}

			new (&corresps[iRow].comparanda[iCol]) Comparandum(wordIPA, wordOrig, NULL);

			if (wordIPA)
				dictinfos[iCol].nWords++;
		}
		dic.ipa->EndSubmitWordForms();
	}
	void FindSameSoundsInRow(Correspondence* crsp)
	{
		//в этой ф-ции назначаются rankAllSoundsSame и nSoundsSame
		//а потом распространяется потенциальный одинаковый звук на все

		bool wasNotEmpty = false;
		Sound* soundSame = NULL,
			*soundSameExact = NULL;

		for (int iCol = 0; iCol < nDicts; iCol++)
		{
			if (crsp->comparanda[iCol].formIPA)
				continue;

			if (!wasNotEmpty)
			{
				soundSameExact = soundSame = crsp->comparanda[iCol].sound;
				crsp->rankAllSoundsSame = 10;
				wasNotEmpty = true;
				crsp->nSoundsSame = 1;
			}
			else if (soundSameExact)
			{
				if (soundSameExact == crsp->comparanda[iCol].sound)
					crsp->nSoundsSame++;
				else if (soundSame && soundSame != crsp->comparanda[iCol].sound)
				{
					Sound* sdBaseWas = dic.ipa->GetBaseSound(soundSame);
					Sound* sdBaseThis = dic.ipa->GetBaseSound(crsp->comparanda[iCol].sound);
					if (sdBaseWas == sdBaseThis)
					{
						crsp->rankAllSoundsSame = 5;
						soundSame = sdBaseWas;
					}
					else
					{
						soundSame = NULL;
						crsp->rankAllSoundsSame = 0;
					}
				}
			}
		}

		if (crsp->nSoundsEmpty && soundSame)
		{
			for (int iCol = 0; iCol < nDicts; iCol++)
			{
				if (!crsp->comparanda[iCol].formIPA)
					crsp->comparanda[iCol].sound = soundSame;
			}
		}
	}
	int CountEmptyColsInRow(Correspondence* crsp)
	{
		crsp->nSoundsEmpty = 0;
		for (int iCol = 0; iCol < nDicts; iCol++)
		{
			if (!crsp->comparanda[iCol].formIPA)
				crsp->nSoundsEmpty++;
		}
		return crsp->nSoundsEmpty;
	}

	void ConfirmOrAnnullMaybeDiphthongs(Correspondence* crsp, bool wasFragment)
	{
		for (int iCol = 0; iCol < nDicts; iCol++)
		{
			if (crsp->comparanda[iCol].typeOfSegment == ST_FRAGMENTMAYBE)
			{
				if (wasFragment)
					crsp->comparanda[iCol].typeOfSegment = ST_FRAGMENT;
				else
					crsp->comparanda[iCol].typeOfSegment = ST_SOUND;
			}
		}
	}

	bool ExtractSoundsFromCognates(Correspondence* crsp, Condition* cnd)
	{
		bool wasFragment = false,
			wasFragmentMaybe = false;
		for (int iCol = 0; iCol < nDicts; iCol++)
		{
			switch (crsp->comparanda[iCol].typeOfSegment = cnd->GetFirstMatchingFragment(
				dic.ipa,
				&crsp->comparanda[iCol].sound,
				crsp->comparanda[iCol].formIPA,
				crsp->comparanda[iCol].chrFragment))
			{
			case ST_ERROR:
				return false;
			case ST_FRAGMENT:
				wasFragment = true;
				break;
			case ST_FRAGMENTMAYBE:
				wasFragmentMaybe = true;
			}
		}

		if ((cnd->sgThis.feature[FT_CLASS] == FT_VOWEL) && wasFragmentMaybe)
			ConfirmOrAnnullMaybeDiphthongs(crsp, wasFragment);

		return true;
	}
	void AcceptAndInsertRow(Correspondence* crsp)
	{
		Correspondence* cFound = (Correspondence*)tCorrespondences.Add(crsp);

		if (cFound)
		{
			crsp->crspNextSame = cFound->crspNextSame;
			cFound->crspNextSame = crsp;
		}
		for (int iCol = 0; iCol < nDicts; iCol++)
		{
			//						corresps[iRow].comparanda[iCol].isSoundInCognates = false;
			if (crsp->comparanda[iCol].formIPA && crsp->comparanda[iCol].sound)
			{
				crsp->comparanda[iCol].isSoundInCognates = true;
				if (cFound)
				{
					cFound->comparanda[iCol].sound = crsp->comparanda[iCol].sound;
					cFound->comparanda[iCol].isSoundInCognates = true;
				}
			}
		}
	}
	void Process(Condition* cnd, bool doRemoveSingleWordsInColumns)
	{
		Reset();

		for (int i_nEmtpy = 0; i_nEmtpy <= nDicts; i_nEmtpy++)
		{
			for (int iRow = 0; iRow < nCorresp; iRow++)
			{
				if (i_nEmtpy != CountEmptyColsInRow(&corresps[iRow]))
					continue;//вроде это не глупость, т.к. сперва надо добавить более полные

				if (!ExtractSoundsFromCognates(&corresps[iRow], cnd))
					continue;

				FindSameSoundsInRow(&corresps[iRow]);

				AcceptAndInsertRow(&corresps[iRow]);
			}
		}

		if (doRemoveSingleWordsInColumns)
			RemoveSingleWordsInColumns();

		CountFilledSoundCorrespondeces();

		isProcessed = true;
	}


	void CalculateDistances(DistanceMatrix* mtx, int factor)
	{
		Correspondence* c;
		for (CorrespondenceTree::Iterator it(&tCorrespondences); c = it.Next();)
		{
			if (it.IsStartOfGroup())
			{
				for (int iRow = 0; iRow < nDicts; iRow++)
				{
					for (int iCol = 0; iCol < nDicts; iCol++)
					{
						mtx->GetDistance(iRow, iCol, &c->comparanda[iRow], &c->comparanda[iCol], factor, true);
					}
				}
			}
			//if (it.IsEndOfGroup())
			//{
			//}
		}
	}
	void CountFilledSoundCorrespondeces()
	{
		Correspondence* c;
		for (CorrespondenceTree::Iterator it(&tCorrespondences); c = it.Next();)
		{
			if (it.IsStartOfGroup())
			{
				nSoundCorresp++;
				for (int iCol = 0; iCol < nDicts; iCol++)
				{
					if (c->comparanda[iCol].isSoundInCognates)
						dictinfos[iCol].nFilledSoundCorresp++;
				}
				it.TryExitGroup();
			}
		}
	}
	void RemoveDistancesIfTooFew(DistanceMatrix* mtx, int threshold)
	{
		for (int i = 0; i < nDicts; i++)
		{
			if (dictinfos[i].nFilledSoundCorresp);

			int percent;
			if (nSoundCorresp)
				percent = (dictinfos[i].nFilledSoundCorresp * 100) / nSoundCorresp;
			else
				percent = 0;

			if (percent < threshold)
			{
				for (int ii = 0; ii < nDicts; ii++)
				{
					mtx->langs[i].dist[ii] = -1;
				}
			}
		}
	}
	void RemoveSingleWordsInColumns()
	{
		int nInCol[100];
		Correspondence* cInCol[100];

		Correspondence* c;
		Correspondence* cStart;
		for (CorrespondenceTree::Iterator it(&tCorrespondences); c = it.Next();)
		{
			if (it.IsStartOfGroup())
			{
				cStart = c;

				//cToDel[ncToDel] = c;
				//ncToDel++;

				for (int iCol = 0; iCol < nDicts; iCol++)
				{
					nInCol[iCol] = 0;
					cInCol[iCol] = NULL;
				}
			}

			for (int iCol = 0; iCol < nDicts; iCol++)
			{
				if (c->comparanda[iCol].formIPA)
				{
					nInCol[iCol]++;
					if (nInCol[iCol] == 1)
						cInCol[iCol] = c;
				}
			}

			if (it.IsEndOfGroup())
			{
				for (int iCol = 0; iCol < nDicts; iCol++)
				{
					if (nInCol[iCol] == 1)
					{
						cInCol[iCol]->comparanda[iCol].formOrig = NULL;
						cInCol[iCol]->comparanda[iCol].formIPA = NULL;
						cInCol[iCol]->comparanda[iCol].translation = NULL;

						cStart->comparanda[iCol].isSoundInCognates = false;
					}
				}
				int nInRow = 0;
				for (int iCol = 0; iCol < nDicts; iCol++)
				{
					if (cStart->comparanda[iCol].formOrig)
					{
						nInRow++;
						if (nInRow > 1)
							break;
					}
				}
				if (nInRow == 1)
					cStart->degenerate = true;
			}
		}
		/*
				while (ncToDel > 0)
				{
					ncToDel--;
					tCorrespondences.Remove(cToDel[ncToDel]);
				}
		*/
	}

	void OutputLanguageList(InfoTree* trOut);
	void SoundCorrespondenceNumbers(InfoTree* trOut);
	void OutputLanguageHeader(InfoTree* trOut, InfoNode* ndTo);
	void OutputPhoneticHeader(InfoTree* trOut, InfoNode* ndTo);
	void OutputSoundsHeader(Correspondence* c, InfoTree* trOut, InfoNode* inTo, bool skipTransl, bool onlyWithForms, int fSep, int fLine);
	void OutputCognatesRow(Correspondence* c, InfoTree* trOut, InfoNode* inTo, bool isPhonData, int fLine);
	void OutputCognate(Comparandum* cmp, InfoTree* trOut, InfoNode* inTo, bool isPhonData, int fLine, CognateList* cl);
	void OutputCognatesBySound(Correspondence* cGroupTop, Correspondence* cOther, int iColDiff, InfoTree* trOut, InfoNode* inMult, InfoTree* trCld, Correspondence* cEqual);
	void OutputDeviationsWithMaterial(Condition* cnd, InfoTree* trOut, InfoTree* trCld);
	void OutputCorresponcesWithMaterial(Condition* cnd, InfoTree* trOut, bool doMakeTableForSingles = false);
	void OutputDistances(Condition* cnd, DistanceMatrix* mtx, InfoTree* trOut);
};

#include "output.h"