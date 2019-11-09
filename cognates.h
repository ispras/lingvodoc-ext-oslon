﻿#pragma once

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
	void* operator new (size_t size, void* memAlloc) { return memAlloc; }

	//Dictionary 			dic;
	//Pool<TCHAR>		pString;

	int			nDicts;
	int 		nCorresp;
	int			nSoundCorresp;
	bool		isProcessed;
	//DictInfo*	dictinfos;
	Condition*	condition;

	//Dictionary*	dics;
	LinkedList<Dictionary> llDicts;


	Correspondence*		corresps;
	CorrespondenceTree 	tCorrespondences;

	Comparison() : tCorrespondences(0)
	{
	}
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

		//dictinfos = (DictInfo*)malloc(nDicts * sizeof(DictInfo));//new DictInfo[nDicts];
		//for (int i = 0; i < nDicts; i++) new (&dictinfos[i]) DictInfo(L"?");

		//dics = new Dictionary[nDicts]
		for (int i = 0; i < nDicts; i++)
		{
			llDicts.Add(new Dictionary);
		}
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
		llDicts.DestroyAll();
	}
	void Reset()
	{
		if (isProcessed)
		{
			tCorrespondences.Empty();

			for (int iRow = 0; iRow < nCorresp; iRow++)
			{
				corresps[iRow]._Reset();

				for (int iCol = 0; iCol < nDicts; iCol++)
					corresps[iRow].comparanda[iCol].Reset_();//почему-то Reset — неоднозначно
			}
			isProcessed = false;
		}
	}
	int AddDictionary(LPTSTR sName)//, int iColNew)
	{
		int iColNew = 0;//только в начало

		nDicts++;
		llDicts.Add(new Dictionary, NULL);
		new (&Dict(0)->dictinfo) DictInfo(sName, NULL);

		for (int iRow = 0; iRow < nCorresp; iRow++)
		{
			corresps[iRow].comparanda = (Comparandum*)realloc(corresps[iRow].comparanda, nDicts * sizeof(Comparandum));//new Comparandum[nDicts];
			for (int i = nDicts - 1; i > iColNew; i--)
			{
				memcpy(&corresps[iRow].comparanda[i], &corresps[iRow].comparanda[i - 1], sizeof(Comparandum));
			}

			new (&corresps[iRow].comparanda[iColNew]) Comparandum(NULL, NULL, NULL, false);
		}
		return nDicts;
	}
	void InitEmpty()
	{
		//for (int iCol = 0; iCol < nDicts; iCol++)
		//	new (&dictinfos[iCol]) DictInfo;
		//???
		for (int iRow = 0; iRow < nCorresp; iRow++)
		{
			new (&corresps[iRow]) Correspondence(nDicts, iRow);
		}
	}
	Dictionary* Dict(int iDict)
	{
		Dictionary* d = llDicts.first;
		for (int i = 0; i < iDict; d = d->next, i++);
		return d;
	}

	void CopyDictionaryFrom(Comparison* cmpFrom, int iColFrom, int iColTo)
	{
		Dictionary* dicTo = Dict(iColTo);
		dicTo->dictinfo.name = cmpFrom->Dict(iColFrom)->dictinfo.name;

		for (int iRow = 0; iRow < nCorresp; iRow++)
		{
			Comparandum* cFrom = &cmpFrom->corresps[iRow].comparanda[iColFrom];
			Comparandum* cTo = &corresps[iRow].comparanda[iColTo];
			new (&corresps[iRow].comparanda[iColTo]) Comparandum(dicTo->TranscribeWord(cFrom->formOrig), dicTo->StoreNonNullString(cFrom->formOrig), NULL, cFrom->isReconstructed);
			if (cTo->formIPA) dicTo->dictinfo.nWords++;
			//{
			//	dic.ipa->SubmitWordForm(cTo->formIPA);
			//	dictinfos[iColTo].nWords++;
			//}

			//НЕ СДЕЛАНО!
			//new (cTo) Comparandum(dic.StoreString(cFrom->formIPA), dic.StoreString(cFrom->formOrig), NULL);
			//в LoadAddress вызывается AddPointerTypeRef и портит тип!!!
		}
		//dic.ipa->EndSubmitWordForms();//это пока не надо, оно только пустые ряды убирает, что сейчас неважно
	}
	//ДУБЛИРУЕТ Dictionary::NextCol
	//должен быть класс InputTable
	bool NextCol(int& iCol, int& iRow, int nCols, int nRows)
	{
		if (iCol == nCols - 1)
		{
			(iRow)++;
			if (iRow >= nRows)
				return false;
			iCol = 0;
		}
		else (iCol)++;

		return true;
	}
	void AddCognateList(LPTSTR sIn, bool hasPhonData, int begCols = 0, int nCols = 0, int nColsAll = 0)
	{
		Parser parser(sIn, L"\0", PARSER_NONNULLEND);
		LPTSTR wordOrig = NULL, wordIPA = NULL, wordTranslation = NULL, wchrTranscr = NULL, wLength = NULL, wF1 = NULL, wF2 = NULL, wF3 = NULL;

		if (!nColsAll) nColsAll = nDicts;
		if (!nCols) nCols = nDicts;

		int iColIn = -1, iCol = -1, iRow = -1;
		while (parser.Next())
		{
			if (!NextCol(iColIn, iRow, nColsAll, nCorresp)) break;
			if (iColIn >= begCols && iColIn < begCols + nCols)
			{
				iCol = iColIn - begCols;

				Dictionary* dic = Dict(iCol);

				if (iRow == -1)
					dic->GetDictInfo(parser);
				else
				{
					if (iCol == 0)
						new (&corresps[iRow]) Correspondence(nDicts, iRow);

					dic->GetOrigIPAAndTranslation(parser, wordOrig, wordIPA, wordTranslation, hasPhonData, wchrTranscr, wLength, wF1, wF2, wF3);
					new (&corresps[iRow].comparanda[iCol]) Comparandum(wordIPA, wordOrig, wordTranslation, false, wchrTranscr, wLength, wF1, wF2, wF3);

					if (wordIPA)
						dic->dictinfo.nWords++;
				}
			}
			else//надо перепрыгнуть через перевод
			{
				parser.Next();
			}
		}

		EndSubmitForms();
	}
	void EndSubmitForms()
	{
		for (Dictionary* d = llDicts.first; d; d = d->next)
			d->ipa->EndSubmitWordForms();
	}
	void AddCognateListText(LPTSTR sIn)
	{
		//LPTSTR _sin = StoreString(sIn);/*это просто копия*/

		Parser parser(sIn, L"\t");
		//Parser parser(_sin, L"\t");
		LPTSTR wordOrig, wordIPA;

		nCorresp = 1; //пока только одна строчка
		while (parser.Next()) nDicts++;

		AllocDictData();

		parser.ResetText(sIn);

		int iRow = -1, iCol = -1;
		while (parser.Next())
		{
			iCol++;

			Dictionary* dic = Dict(iCol);

			wordOrig = dic->StoreString(parser.Current(), parser.LengthOfCurrentWord());
			wordIPA = dic->TranscribeWord(wordOrig);

			if (iCol == 0)
			{
				iRow++;
				new (&corresps[iRow]) Correspondence(nDicts, iRow);
			}

			new (&corresps[iRow].comparanda[iCol]) Comparandum(wordIPA, wordOrig, NULL, false);

			if (wordIPA)
				dic->dictinfo.nWords++;
		}
		EndSubmitForms();
	}
	bool FillEmptySoundsInRow(Correspondence* crsp)//, bool isReadding = false)
	{

		//в этой ф-ции назначаются rankAllSoundsSame и nSoundsSame
		//а потом распространяется потенциальный одинаковый звук на все
		//если нет одинакового, то суётся первый

/*
		if (isReadding)
		{
			for (int iCol = 0; iCol < nDicts; iCol++)
			{
				//if (crsp->comparanda[iCol].typeOfSegment == ST_EMPTYAUTOFILL)
				//	crsp->comparanda[iCol].sound = NULL;
			}
		}
*/
		bool wasNotEmpty = false;
		Sound* soundSame = NULL,
			*soundSameExact = NULL,
			*soundFirst = NULL;
		int typWas = ST_ERROR;
		for (int iCol = 0; iCol < nDicts; iCol++)
		{
			Dictionary* dic = Dict(iCol);

			if (!crsp->comparanda[iCol].sound || crsp->comparanda[iCol].typeOfSegment == ST_EMPTYAUTOFILL)
				continue;

			if (typWas != ST_ERROR && crsp->comparanda[iCol].typeOfSegment != typWas)
			{
				crsp->rankAllSoundsSame = 0;
				break;
			}
			typWas = crsp->comparanda[iCol].typeOfSegment;

			if (!soundFirst)
				soundFirst = crsp->comparanda[iCol].sound;

			if (!wasNotEmpty)
			{
				soundSameExact = soundSame = crsp->comparanda[iCol].sound;
				crsp->rankAllSoundsSame = 10;
				wasNotEmpty = true;
				crsp->nSoundsSame = 1;
			}
			else if (soundSameExact)
			{
				if (!dic->ipa->CompareSoundsByText(soundSameExact, crsp->comparanda[iCol].sound))
					crsp->nSoundsSame++;
				else if (soundSame && soundSame != crsp->comparanda[iCol].sound)
				{
					Sound* sdBaseWas = dic->ipa->GetBaseSound(soundSame);
					Sound* sdBaseThis = dic->ipa->GetBaseSound(crsp->comparanda[iCol].sound);

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

		if (crsp->nSoundsEmpty)
		{
			if (!soundSame)
				soundSame = soundFirst;

			if (!soundSame)//строка выродилась
			{
				return false;
			}

			for (int iCol = 0; iCol < nDicts; iCol++)
			{
				if (!crsp->comparanda[iCol].sound/*formIPA*/)
				{
					crsp->comparanda[iCol].sound = soundSame;
					crsp->comparanda[iCol].typeOfSegment = ST_EMPTYAUTOFILL;
				}
			}
		}

		return true;
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
	bool ExtractSoundsFromCognates(Correspondence* crsp, Condition* cnd)
	{
		bool wasFragment = false,
			wasFragmentMaybe = false;
		for (int iCol = 0; iCol < nDicts; iCol++)
		{
			switch (crsp->comparanda[iCol].typeOfSegment = cnd->GetFirstMatchingFragment(
				Dict(iCol)->ipa,
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
		for (int iCol = 0; iCol < nDicts; iCol++)
		{
			if (crsp->comparanda[iCol].sound && crsp->comparanda[iCol].typeOfSegment != ST_EMPTYAUTOFILL)
				crsp->comparanda[iCol].isSoundInCognates = true;
		}


		Correspondence* c;
		CorrespondenceTree::COMPAREFLAGS cf = { true, false, false };

		for (CorrespondenceTree::Iterator it(&tCorrespondences); c = it.Next();)
		{
			if (tCorrespondences.CompareNodes(c, crsp, &cf))
				it.TryExitGroup();
			else
			{
				crsp->AddToGroup(c);
				break;
			}
		}


		if (!crsp->crspMain)
		{
			if (tCorrespondences.Add(crsp))
				;//	out("УЖЕ ЕСТЬ!");
		}
		else
			FillMainRowWithMissingSounds(crsp);//, false);
	}
	void FillMainRowWithMissingSounds(Correspondence* crsp)//, bool doAlterTree)
	{
		bool wasChange = false;
		for (int iCol = 0; iCol < nDicts; iCol++)
		{
			Comparandum* cThis = &crsp->comparanda[iCol];
			if (/*cThis->formIPA &&*/ cThis->sound && cThis->typeOfSegment != ST_EMPTYAUTOFILL)
			{
				Comparandum* cMain = &crsp->crspMain->comparanda[iCol];
				if (!cMain->sound || cMain->typeOfSegment == ST_EMPTYAUTOFILL)
				{

					if (!wasChange)
					{
						wasChange = true;

						tCorrespondences.Remove(crsp->crspMain);
					}

					cMain->sound = cThis->sound;
					cMain->typeOfSegment = cThis->typeOfSegment;
					wcscpy(cMain->chrFragment, cThis->chrFragment);

					cMain->isSoundInCognates = true;
				}
			}
		}

		if (wasChange)
			ReaddRow(crsp->crspMain, false);
	}
	void Process(Condition* cnd, bool doRemoveSingleWordsInColumns, bool doConflate)
	{
		Reset();

		for (int i_nEmtpy = 0; i_nEmtpy <= nDicts; i_nEmtpy++)
		{
			for (int iRow = 0; iRow < nCorresp; iRow++)
			{
				if (i_nEmtpy != CountEmptyColsInRow(&corresps[iRow]))//вроде это не глупость, т.к. сперва надо добавить более полные
					continue;

				if (!ExtractSoundsFromCognates(&corresps[iRow], cnd))
					continue;

				if (!FillEmptySoundsInRow(&corresps[iRow])) //значит пусто совсем
					continue;

				AcceptAndInsertRow(&corresps[iRow]);
			}
		}
		if (doRemoveSingleWordsInColumns)
			RemoveSingleWordsInColumns();
		if (doConflate)
			ConflateRows();

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
						Dict(iCol)->dictinfo.nFilledSoundCorresp++;
				}
				it.TryExitGroup();
			}
		}
	}
	void ReaddRow(Correspondence* crsp, bool isUnique)
	{
		if (isUnique)
			crsp->iUnique = tCorrespondences.GetUniqueID();

		if (!FillEmptySoundsInRow(crsp))//, true))
		{
			//			out(L"выродилось!");
			//			outrow(crsp, false, true);
			return;
		}

		Correspondence* cFound = (Correspondence*)tCorrespondences.Add(crsp);
		if (cFound)
		{
			//			out(L"не передобавился ряд:");
			//			outrow(crsp);
			//			out(L"ибо был такой:");
			//			outrow(cFound);
		}
		//а лучше сразу добавлять их как-то правильно, а потом ещё раз, что ль, фильтровать???
		//иначе все звуки стали нулями — но это временно! надо не ...
	}

	void RemoveDistancesIfTooFew(DistanceMatrix* mtx, int threshold)
	{
		for (int i = 0; i < nDicts; i++)
		{
			//if (dictinfos[i].nFilledSoundCorresp);

			int percent;
			if (nSoundCorresp)
				percent = (Dict(i)->dictinfo.nFilledSoundCorresp * 100) / nSoundCorresp;
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
	/*
		void UnsetSameSoundsInRow(Correspondence* crsp)
		{
			for (int iCol = 0; iCol < nDicts; iCol++)
			{
				if (crsp->comparanda[iCol].typeOfSegment == ST_EMPTYAUTOFILL)
				{
					crsp->comparanda[iCol].typeOfSegment = ST_NONE;
					crsp->comparanda[iCol].sound = NULL;
				}
			}
		}
	*/
	void CopySoundsToRow(Correspondence* cFrom, Correspondence* cTo)
	{
		for (int iCol = 0; iCol < nDicts; iCol++)
		{
			cTo->comparanda[iCol].isSoundInCognates = cFrom->comparanda[iCol].isSoundInCognates;
			if (!cTo->comparanda[iCol].isSoundInCognates && !cTo->comparanda[iCol].formOrig)
			{
				cTo->comparanda[iCol].sound = NULL;
				cTo->comparanda[iCol].typeOfSegment = ST_NONE;
			}
			//cTo->comparanda[iCol].sound = cFrom->comparanda[iCol].sound;
			//cTo->comparanda[iCol].typeOfSegment = cFrom->comparanda[iCol].typeOfSegment;
			//cTo->comparanda[iCol].isSingleInGroup = cFrom->comparanda[iCol].isSingleInGroup;
			//wcscpy(cTo->comparanda[iCol].chrFragment, cFrom->comparanda[iCol].chrFragment);
		}
	}
	void SetSingleColsToNull(Correspondence* cMain)
	{
		//tCorrespondences.Remove(cMain);

		for (int iCol = 0; iCol < nDicts; iCol++)
		{
			if (cMain->comparanda[iCol].isSingleInGroup)
			{
				cMain->comparanda[iCol].isSoundInCognates = false;

				if (!cMain->comparanda[iCol].formIPA)
				{//т.е. обнуляем только те, что были добавлены ниже
					cMain->comparanda[iCol].sound = NULL;
					cMain->comparanda[iCol].typeOfSegment = ST_NONE;
				}
			}
		}
		//tCorrespondences.Add(cMain);
	}
	void SetNextRowAsGroupHead(Correspondence* cMain)
	{
		Correspondence* cFirst = cMain->first;

		tCorrespondences.Remove(cMain);

		SetSingleColsToNull(cMain);

		CopySoundsToRow(cMain, cFirst);

		for (Correspondence* cNext = cFirst->next; cNext; cNext = cNext->next)
			cNext->crspMain = cFirst;

		cFirst->first = cFirst->next;
		cFirst->last = cMain->last;
		cMain->first = cMain->last = NULL;
		cFirst->crspMain = NULL;

		ReaddRow(cMain, true);
		ReaddRow(cFirst, true);
	}
	void RemoveSingleWordsInColumns()
	{
		while (RemoveSingleWordsInColumnsOnce());
		//RemoveSingleWordsInColumnsOnce();
	}
	int RemoveSingleWordsInColumnsOnce()
	{
		int* nInCol = new int[nDicts];
		Correspondence** cInCol = new Correspondence*[nDicts];
		Correspondence** cToDel = new Correspondence*[nCorresp];
		int ncToDel = 0;

		Correspondence* c;
		Correspondence* cStart = NULL;
		for (CorrespondenceTree::Iterator it(&tCorrespondences); c = it.Next();)
		{
			if (it.IsStartOfGroup())
			{
				cStart = c;

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
					cInCol[iCol] = c;
				}
			}

			if (it.IsEndOfGroup())
			{
				for (int iCol = 0; iCol < nDicts; iCol++)
				{
					if (nInCol[iCol] == 1)
					{
						bool isAlreadyAddedForDelete = false;
						for (int iiCol = 0; iiCol < nDicts; iiCol++)
						{
							if (cStart->comparanda[iiCol].isSingleInGroup)
							{
								isAlreadyAddedForDelete = true;
								break;
							}
						}

						cStart->comparanda[iCol].isSingleInGroup = true;

						if (!isAlreadyAddedForDelete)
						{
							cToDel[ncToDel] = cInCol[iCol];
							ncToDel++;
						}
					}
				}
			}
		}

		for (int i = 0; i < ncToDel; i++)
		{
			Correspondence* cMain = cToDel[i]->crspMain;
			if (cMain) //т.е. в группе
			{
				cToDel[i]->RemoveFromGroup();
				//ReaddRow(cToDel[i], true);
				cToDel[i]->iUnique = tCorrespondences.GetUniqueID();

				if (tCorrespondences.Add(cToDel[i]))
					;//out(L"ААА!");
//out(L"убрали из группы и вставили в древо:");
//outrow(cToDel[i]);

				if (!cMain->isBeingChanged)
				{
					//out(L"меняем заголовок ИЗ группы!");
					//outrow(cMain);
					cMain->isBeingChanged = true;
					if (!tCorrespondences.Remove(cMain))
						;//out(L"ООООО!");

					SetSingleColsToNull(cMain);

					ReaddRow(cMain, false);
				}
			}
			else  //т.е. заголовок группы
			{
				cMain = cToDel[i];
				if (!cMain->first)
					;//ничего не делать, она сама теперь не заголовок, т.к. вся группа ушла
				else
				{
					//out(L"убираем САМ заголовок");
					//outrow(cMain);
					SetNextRowAsGroupHead(cMain);
				}
			}
		}

		for (CorrespondenceTree::Iterator it(&tCorrespondences); c = it.Next();)
		{
			//if (it.IsStartOfGroup())
			c->isBeingChanged = false;
			for (int iCol = 0; iCol < nDicts; iCol++)
				c->comparanda[iCol].isSingleInGroup = false;
		}


		delete[] nInCol;
		delete[] cInCol;
		delete[] cToDel;

		return ncToDel;
	}
	void ConflateRows()
	{
		Correspondence* c1,
			*c2;
		//CorrespondenceTree::COMPAREFLAGS cf = {false, true, true};
		CorrespondenceTree::COMPAREFLAGS cf = { true, true, true };
	Anew:
		for (CorrespondenceTree::Iterator it1(&tCorrespondences); c1 = it1.Next();)
		{
			if (it1.IsStartOfGroup())
			{
				if (!c1->isBeingChanged)
				{
					bool wasThis = false;
					for (CorrespondenceTree::Iterator it2(&tCorrespondences); c2 = it2.Next();)
					{
						if (it2.IsStartOfGroup())
						{
							if (!wasThis)
								wasThis = (c1 == c2);
							else
							{
								if (!tCorrespondences.CompareNodes(c1, c2, &cf))
								{
									//out(L"включаем в группу ряд:");
									//outrow(c1);
									//out(L"вот в эту:");
									//outrow(c2);
									//outallrows(c1,c2);
									tCorrespondences.Remove(c1);

									c1->AddToGroup(c2);

									FillMainRowWithMissingSounds(c1);//, true);

									//cToDel[ncToDel] = c1;
									//ncToDel++;
//outallrows(NULL, c1->crspMain);
									goto Anew;
								}
							}
							it2.TryExitGroup();
						}
					}
					//не нашли, помечаем как отработанное
					c1->isBeingChanged = true;
				}
				//	NextC1:;
				it1.TryExitGroup();
			}
		}

		//		for (int i = 0; i < ncToDel; i++)
		//		{
		//			tCorrespondences.Remove(cToDel[i]);
		//		}

		//		delete[] cToDel;
		for (CorrespondenceTree::Iterator it(&tCorrespondences); c1 = it.Next();)
			c1->isBeingChanged = false;
	}

	void OutputLanguageList(InfoTree* trOut);
	void SoundCorrespondenceNumbers(InfoTree* trOut, int threshold);
	void OutputLanguageHeader(InfoTree* trOut, InfoNode* ndTo, bool isProtoSounds);
	void OutputPhoneticHeader(InfoTree* trOut, InfoNode* ndTo);
	void OutputSoundsHeader(Correspondence* c, InfoTree* trOut, InfoNode* inTo, bool skipTransl, bool onlyWithForms, int fSep, int fLine);
	void OutputCognatesRow(Correspondence* c, InfoTree* trOut, InfoNode* inTo, bool isPhonData, int fLine);
	void OutputCognate(Comparandum* cmp, InfoTree* trOut, InfoNode* inTo, bool isPhonData, int fLine, CognateList* cl);
	void OutputCognatesBySound(Correspondence* cGroupTop, Correspondence* cOther, int iColDiff, InfoTree* trOut, InfoNode* inMult, InfoTree* trCld, Correspondence* cEqual);
	void OutputDeviationsWithMaterial(Condition* cnd, InfoTree* trOut, InfoTree* trCld);
	void OutputCorrespondencesWithMaterial(Condition* cnd, InfoTree* trOut, bool doMakeTableForSingles = false);
	void OutputReconstructedSounds(Condition* cnd, InfoTree* trOut);
	void OutputReconstructedWords(InfoTree* trOut);
	void OutputDistances(Condition* cnd, DistanceMatrix* mtx, InfoTree* trOut);


};

#include "output.h"

