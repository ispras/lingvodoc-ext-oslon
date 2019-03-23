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


class Comparison
{
public:
	Dictionary 		dic;
	//Pool<TCHAR>		pString;

	bool			isProcessed;

	DictInfo*		dictinfos;

	class Comparandum : public OwnNew
	{
	public:
		LPTSTR 		formIPA;
		LPTSTR 		formOrig;
		LPTSTR 		translation;
		LPTSTR		wLength, wF1, wF2, wF3;
		Sound*		sound;
		TCHAR		chrTranscr[8];
		bool		isSoundInCognates;
		Comparandum(LPTSTR _formIPA, LPTSTR _formOrig, LPTSTR _translation, LPTSTR _chrTranscr = NULL, LPTSTR _wLength = NULL, LPTSTR _wF1 = NULL, LPTSTR _wF2 = NULL, LPTSTR _wF3 = NULL)
		{
			formIPA = _formIPA;
			formOrig = _formOrig;
			translation = _translation;

			wF1 = _wF1;
			wF2 = _wF2;
			wF3 = _wF3;
			wLength = _wLength;

			if (_chrTranscr)
				StrCpyWMax(chrTranscr, _chrTranscr, 8);
			else
				chrTranscr[0] = L'\0';

			Reset_();
		}
		void Reset_()
		{
			sound = NULL;
			isSoundInCognates = false;
		}
	};

	class Correspondence : public BNode
	{
	public:
		int				iRow;
		Correspondence*	crspNextSame;
		Comparandum*	comparanda;
		int				rankAllSoundsSame;
		int				nSoundsSame;
		int				nSoundsEmpty;
		void*			dataExtra;
		bool			degenerate;

		Correspondence(int nDicts, int _iRow)
		{
			iRow = _iRow;
			crspNextSame = NULL;
			comparanda = (Comparandum*)malloc(nDicts * sizeof(Comparandum));//new Comparandum[nDicts];
			memset(comparanda, '\0', nDicts * sizeof(Comparandum));

			rankAllSoundsSame = 0;
			nSoundsSame = 0;
			nSoundsEmpty = 0;
			degenerate = false;

			dataExtra = NULL;
		}
		~Correspondence()
		{
			if (comparanda)
			{
				free(comparanda);//delete[] comparanda;
			}
		}
	}*corresps;

	class CorrespondenceTree : public BTree
	{
	public:
		int nDicts;
		CorrespondenceTree(int _nDicts)
		{
			nDicts = _nDicts;
		}
		int CompareNodes(BNode* _nd1, BNode* _nd2, void*_)
		{
			Correspondence* c1 = (Correspondence*)_nd1;//поэтому надо шаблонно!!
			Correspondence* c2 = (Correspondence*)_nd2;

			if (c1->rankAllSoundsSame > c2->rankAllSoundsSame)
				return -1;
			if (c1->rankAllSoundsSame < c2->rankAllSoundsSame)
				return 1;

			bool isNonNull = false;
			for (int iCol = 0; iCol < nDicts; iCol++)
			{
				int res;
				if (c1->comparanda[iCol].sound && c2->comparanda[iCol].sound)
				{
					//if (!c1->comparanda[iCol].sound && c2->comparanda[iCol].sound)
					//	return 1;
					//if (c1->comparanda[iCol].sound && !c2->comparanda[iCol].sound)
					//	return -1;

					isNonNull = true;
					if (res = CompareFeaturesAnd(c1->comparanda[iCol].sound->feature, c2->comparanda[iCol].sound->feature))
						return res;
				}
			}
			/*
						for (int iCol = 0; iCol < nDicts ; iCol++)
						{
							int res;
							if (!c1->comparanda[iCol].sound && c2->comparanda[iCol].sound)
								return -1;
							if (c1->comparanda[iCol].sound && !c2->comparanda[iCol].sound)
								return 1;
						}
			*/
			if (!isNonNull)//т.е. если ни по одному столбцу не сравнили
			{
				if (c1->iRow < c2->iRow)
					return 1;
				if (c1->iRow > c2->iRow)
					return -1;
			}

			return 0;
		}

		class Iterator : public BTree::Walker
		{
			bool			isInGroup;
			bool			skipInsideGroup;
			Correspondence*	crspCurrInGroup;
		public:
			Iterator(CorrespondenceTree* _tree, bool _skipInsideGroup = false) : Walker(_tree)
			{
				skipInsideGroup = _skipInsideGroup;
				isInGroup = false;
				crspCurrInGroup = NULL;
			}

			bool TryEnterGroup()
			{
				if (!isInGroup)
				{
					Correspondence* crspCurr = (Correspondence*)Current();
					if (crspCurr->crspNextSame)
					{
						isInGroup = true;
						crspCurrInGroup = crspCurr;
					}
				}
				return isInGroup;
			}
			void TryExitGroup()
			{
				isInGroup = false;
				crspCurrInGroup = NULL;
			}
			bool IsStartOfGroup()
			{
				if (skipInsideGroup)
				{
					Correspondence* crspCurr = (Correspondence*)Current();
					if (!crspCurr)
						return false;
					return (crspCurr->crspNextSame);
				}
				else
				{
					return (crspCurrInGroup == Current());
				}
			}
			bool IsEndOfGroup()
			{
				if (!isInGroup) return false;

				return (crspCurrInGroup->crspNextSame == NULL);
			}
			bool AreWeInsideGroup()
			{
				return isInGroup;
			}
			void NextAndCheck()
			{
			TryAgain:
				if (!Walker::Next())
					return;

				Correspondence* crspCurr = (Correspondence*)Current();

				if (crspCurr->degenerate)
					goto TryAgain;

				if (!skipInsideGroup)
				{
					TryEnterGroup();
				}
			}
			Correspondence* Next()
			{
				if (!Current() || !isInGroup)
				{
					NextAndCheck();
				}
				else if (!(crspCurrInGroup = crspCurrInGroup->crspNextSame))
				{
					isInGroup = false;
					NextAndCheck();
				}

				if (isInGroup)
					return crspCurrInGroup;
				else
					return (Correspondence*)Current();
			}
		};
	};

	CorrespondenceTree	tCorrespondences;

	int					nDicts;
	int 				nCorresp;


	Comparison(int nRows, int nCols) : tCorrespondences(nCols)
	{
		nDicts = nCols;
		nCorresp = nRows;
		//corresps = new Correspondence[nRows];
		corresps = (Correspondence*)malloc(nRows * sizeof(Correspondence));

		dictinfos = (DictInfo*)malloc(nDicts * sizeof(DictInfo));//new DictInfo[nDicts];
		for (int i = 0; i < nDicts; i++) new (&dictinfos[i]) DictInfo(L"?");

		isProcessed = false;
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
			}
		}
		dic.ipa->EndSubmitWordForms();
	}
	void Process(Condition* cnd, bool doRemoveSingleWordsInColumns)
	{
		Reset();

		Segmentizer* sgmntzr = (Segmentizer*)malloc(nDicts * sizeof(Segmentizer));

		for (int i_nEmtpy = 0; i_nEmtpy <= nDicts; i_nEmtpy++)
		{
			for (int iRow = 0; iRow < nCorresp; iRow++)
			{
				int nEmpty = 0;
				for (int iCol = 0; iCol < nDicts; iCol++)
				{
					if (!corresps[iRow].comparanda[iCol].formIPA)
						nEmpty++;
				}

				if (i_nEmtpy == nEmpty)//глупость!! надо просто сортировать с учётом числа пустых
				{
					corresps[iRow].nSoundsEmpty = nEmpty;

					for (int iCol = 0; iCol < nDicts; iCol++)
					{
						new (&sgmntzr[iCol]) Segmentizer(dic.ipa, corresps[iRow].comparanda[iCol].formIPA);
						Sound* sound;
						cnd->Reset();

						if (!corresps[iRow].comparanda[iCol].formIPA)
							sound = NULL;
						else
						{
							Sound* sdCur;
							bool isYes = false;
							while (sdCur = sgmntzr[iCol].GetNext())
							{
								isYes |= cnd->Check(&sgmntzr[iCol]);
								if (isYes) break; //else if (!isYes && QR_FIRSTINWORD)
							}

							if (!isYes) goto NextCorrespondence;


							sound = sgmntzr[iCol].Current();
						}
						corresps[iRow].comparanda[iCol].sound = sound;
					}


					bool wasNotEmpty = false;
					Sound* soundSame = NULL,
						*soundSameExact = NULL;

					for (int iCol = 0; iCol < nDicts; iCol++)
					{
						if (!corresps[iRow].comparanda[iCol].formIPA)
							continue;

						if (!wasNotEmpty)
						{
							soundSameExact = soundSame = corresps[iRow].comparanda[iCol].sound;
							corresps[iRow].rankAllSoundsSame = 10;
							wasNotEmpty = true;
							corresps[iRow].nSoundsSame = 1;
						}
						else if (soundSameExact)
						{
							if (soundSameExact == corresps[iRow].comparanda[iCol].sound)
								corresps[iRow].nSoundsSame++;
							else if (soundSame && soundSame != corresps[iRow].comparanda[iCol].sound)
							{
								Sound* sdBaseWas = dic.ipa->GetBaseSound(soundSame);
								Sound* sdBaseThis = dic.ipa->GetBaseSound(corresps[iRow].comparanda[iCol].sound);
								if (sdBaseWas == sdBaseThis)
								{
									corresps[iRow].rankAllSoundsSame = 5;
									soundSame = sdBaseWas;
								}
								else
								{
									soundSame = NULL;
									corresps[iRow].rankAllSoundsSame = 0;
								}
							}
						}
					}
					if (nEmpty && soundSame)
					{
						for (int iCol = 0; iCol < nDicts; iCol++)
						{
							if (!corresps[iRow].comparanda[iCol].formIPA)
								corresps[iRow].comparanda[iCol].sound = soundSame;
						}
					}
					//corresps[iRow].rankAllSoundsSame = rankAllSoundsSame;

					Correspondence* cFound = (Correspondence*)tCorrespondences.Add(&corresps[iRow]);
					if (cFound)
					{
						corresps[iRow].crspNextSame = cFound->crspNextSame;
						cFound->crspNextSame = &corresps[iRow];
					}
					for (int iCol = 0; iCol < nDicts; iCol++)
					{
						//						corresps[iRow].comparanda[iCol].isSoundInCognates = false;
						if (corresps[iRow].comparanda[iCol].formIPA && corresps[iRow].comparanda[iCol].sound)
						{
							corresps[iRow].comparanda[iCol].isSoundInCognates = true;
							if (cFound)
							{
								cFound->comparanda[iCol].sound = corresps[iRow].comparanda[iCol].sound;
								cFound->comparanda[iCol].isSoundInCognates = true;
							}
						}
					}
				}
			NextCorrespondence:
				;
			}
			//if (isAddingWithEmpty) break;
		}
		delete sgmntzr;

		if (doRemoveSingleWordsInColumns)
			RemoveSingleWordsInColumns();

		isProcessed = true;
	}


	void OutputLanguageList(InfoTree* trOut)
	{
		TCHAR buf[500];
		int f = 0;//IT_EMPTYLINEBEFORE;
		trOut->Add(NULL, IT_HORLINE);
		for (int i = 0; i < nDicts; i++)
		{
			_ltow(i + 1, buf, 10);
			wcscat(buf, L": ");
			if (dictinfos[i].name)
				wcscat(buf, dictinfos[i].name);
			trOut->Add(buf, IT_LINEBRKAFTER | f);
			//trOut->Add(IT_LINEBRK, ndTo);
			f = 0;
		}
		trOut->Add(NULL, IT_HORLINE);
	}
	void OutputLanguageHeader(InfoTree* trOut, InfoNode* ndTo)
	{
		LPTSTR word;
		trOut->Add(NULL, IT_HORLINE, ndTo);
		for (int iCol = 0; iCol < nDicts; iCol++)
		{
			trOut->Add(dictinfos[iCol].name, IT_TAB, ndTo);
			trOut->Add(L"", IT_TAB, ndTo);
		}
		trOut->Add(NULL, IT_HORLINE, ndTo);
	}
	void OutputPhoneticHeader(InfoTree* trOut, InfoNode* ndTo)
	{
		trOut->Add(NULL, IT_HORLINE, ndTo);
		trOut->Add(L"словоформа", IT_TAB, ndTo);
		trOut->Add(L"перевод", IT_TAB, ndTo);
		trOut->Add(L"гласный", IT_TAB, ndTo);
		trOut->Add(L"t", IT_TAB, ndTo);
		trOut->Add(L"f₁", IT_TAB, ndTo);
		trOut->Add(L"f₂", IT_TAB, ndTo);
		trOut->Add(L"f₃", IT_TAB, ndTo);
		trOut->Add(NULL, IT_HORLINE, ndTo);
	}
	void OutputSoundsHeader(Correspondence* c, InfoTree* trOut, InfoNode* inTo, bool skipTransl, bool onlyWithForms, int fSep, int fLine)
	{
		for (int iCol = 0; iCol < nDicts; iCol++)
		{
			LPTSTR word;

			int fAdd;
			if (iCol < nDicts - 1)
				fAdd = fSep;
			else
				fAdd = 0;

			bool isSoundOK;
			if (onlyWithForms)
				isSoundOK = !!c->comparanda[iCol].formIPA;
			else
				isSoundOK = c->comparanda[iCol].isSoundInCognates;

			if (isSoundOK)
			{
				fAdd |= IT_SQRBRK;
				word = c->comparanda[iCol].sound->Symbol;
			}
			else word = L" ? ";


			trOut->Add(word, fAdd, inTo);

			if (skipTransl)
				trOut->Add(L"", IT_TAB, inTo);
		}
		trOut->Add(NULL, fLine, inTo);
	}
	void OutputCognatesRow(Correspondence* c, InfoTree* trOut, InfoNode* inTo, bool isPhonData, int fLine)
	{
		for (int iCol = 0; iCol < nDicts; iCol++)
		{
			OutputCognate(&c->comparanda[iCol], trOut, inTo, isPhonData, fLine, NULL);
		}
		if (fLine)
			trOut->Add(NULL, fLine, inTo);
	}
	class CognateList
	{
	public:
		float l_, f1_, f2_, f3_;
		TCHAR L[30];
		TCHAR F1[30];
		TCHAR F2[30];
		TCHAR F3[30];
		int n;
		InfoTree* trOut;
		InfoNode *inMult;
		InfoTree* trCld;

		CognateList(InfoTree* _trOut, InfoNode *_inMult, InfoTree* _trCld = NULL)
		{
			trOut = _trOut;
			inMult = _inMult;
			trCld = _trCld;
			l_ = f1_ = f2_ = f3_ = 0;
			n = 0;
		}
		void Add(LPTSTR l, LPTSTR f1, LPTSTR f2, LPTSTR f3)
		{
			if (!l || !f1 || !f2 || !f3)
				return;

			_addcalc(&l_, l);
			_addcalc(&f1_, f1);
			_addcalc(&f2_, f2);
			_addcalc(&f3_, f3);

			if (trCld)
			{
				trCld->Add(l, IT_TAB);
				trCld->Add(f1, IT_TAB);
				trCld->Add(f2, IT_TAB);
				trCld->Add(f3, IT_TAB);
			}

			n++;
		}
		void Done()
		{
			_donecalc(&l_, L, n);
			_donecalc(&f1_, F1, n);
			_donecalc(&f2_, F2, n);
			_donecalc(&f3_, F3, n);
		}
		void Output()
		{
			Done();
			trOut->Add(L"Средние", IT_COLUMN | IT_TAB, inMult);
			trOut->Add(NULL, IT_TAB, inMult);
			trOut->Add(NULL, IT_TAB, inMult);
			trOut->Add(L, IT_TAB, inMult);
			trOut->Add(F1, IT_TAB, inMult);
			trOut->Add(F2, IT_TAB, inMult);
			trOut->Add(F3, IT_TAB, inMult);
		}
	};
	void OutputCognate(Comparandum* cmp, InfoTree* trOut, InfoNode* inTo, bool isPhonData, int fLine, CognateList* cl)
	{
		trOut->Add(cmp->formOrig, IT_TAB, inTo);
		trOut->Add(cmp->translation, IT_MARRQUOTES | IT_TAB, inTo);

		if (isPhonData)
		{
			trOut->Add(cmp->chrTranscr, IT_TAB, inTo);
			trOut->Add(cmp->wLength, IT_TAB, inTo);
			trOut->Add(cmp->wF1, IT_TAB, inTo);
			trOut->Add(cmp->wF2, IT_TAB, inTo);
			trOut->Add(cmp->wF3, IT_TAB, inTo);

			if (cl)
			{
				cl->Add(cmp->wLength, cmp->wF1, cmp->wF2, cmp->wF3);
			}
		}

		if (fLine)
			trOut->Add(NULL, fLine, inTo);
	}

	void OutputCognatesBySound(Correspondence* cGroupTop, Correspondence* cOther, int iColDiff, InfoTree* trOut, InfoNode* inMult, InfoTree* trCld, Correspondence* cEqual)
	{
		if (trCld)
		{

			if (!cEqual->comparanda[iColDiff].sound)
				trCld->Add(L"?", IT_SPACE);
			else
				trCld->Add(cEqual->comparanda[iColDiff].sound->Symbol, IT_SQRBRK | IT_SPACE);
			trCld->Add(L"в других рядах", IT_LINEBRKAFTER);
		}


		bool isRegExtra = false;
		bool isRows = false;

		Correspondence* cExtra;
		for (CorrespondenceTree::Iterator itExtra(&tCorrespondences, true); cExtra = itExtra.Next();) //всё верно
		{
			if (cExtra == cGroupTop)
				continue;
			if (cExtra == cOther)
				continue;

			if (cExtra->comparanda[iColDiff].sound == cEqual->comparanda[iColDiff].sound)
			{
				isRegExtra = false;

				if (itExtra.IsStartOfGroup())
				{
					CognateList cl(trOut, inMult, trCld);
					for (itExtra.TryEnterGroup(); cExtra; cExtra = itExtra.Next())
					{
						if (cExtra->comparanda[iColDiff].formOrig)
						{
							if (!isRegExtra)
							{
								isRegExtra = true;

								trOut->Add(cEqual->comparanda[iColDiff].sound->Symbol, IT_SQRBRK | IT_SPACE, inMult);
								trOut->Add(L"также в ряду", IT_COLUMN | IT_TAB, inMult);
								OutputSoundsHeader(cExtra, trOut, inMult, false, false, IT_DASH, IT_HORLINE);
							}

							OutputCognate(&cExtra->comparanda[iColDiff], trOut, inMult, true, IT_LINEBRK, &cl);
						}
						if (itExtra.IsEndOfGroup())
							break;
					};
					if (isRegExtra)
					{
						trOut->Add(NULL, IT_HORLINE, inMult);
						if (cl.n > 1)
						{
							cl.Output();
							trOut->Add(NULL, IT_HORLINE, inMult);
						}
						if (cl.n > 0)
							isRows = true;
					}
				}
			}

			//			if (it.IsEndOfGroup())
			//			{
			//				trOut->Add(NULL, IT_HORLINE, inMult);
			//				inTo = inOnce;
			//			}
		}

		//		trOut->Add(NULL, IT_HORLINE, inMult);

		if (trCld && !isRows)
			trCld->Add(NULL, IT_EMPTYLINEBEFORE);

	}

	void OutputDeviationsWithMaterial(Condition* cnd, InfoTree* trOut, InfoTree* trCld)
	{
		InfoNode* inCnd, *inMult;//, *inTo;//, *inOnce, *inMultList;

		inMult = inCnd = trOut->Add(cnd->title, IT_COLUMN | IT_LINEBRKBEFORE | IT_LINEBRKAFTER, NULL, false, cnd);

		OutputPhoneticHeader(trOut, inMult);

		int nDeviationGroups = 0;
		int nDeviations;
		TCHAR bufn[300];

		Correspondence* cGroup, *cGroupTop;
		for (CorrespondenceTree::Iterator itGroup(&tCorrespondences, true); cGroupTop = cGroup = itGroup.Next();) //всё верно, надо сразу на следующий прыгать (он сразу будет первым)
		{
			if (cGroup->dataExtra)//уже был в отклонениях (т.е. в ряду №2)
				continue;
			if (!itGroup.IsStartOfGroup())
				continue;

			bool isDeviations = false;

			Correspondence* cOther;
			for (CorrespondenceTree::Iterator itOther(&tCorrespondences, true); cOther = itOther.Next();) //всё верно
			{

				int nDiff = 0, iColDiff;
				for (int iCol = 0; (iCol < nDicts) && nDiff <= 1; iCol++)
				{
					if (cOther->comparanda[iCol].formIPA)
					{
						if (cGroupTop->comparanda[iCol].sound != cOther->comparanda[iCol].sound)
						{
							nDiff++;
							iColDiff = iCol;
						}
					}
				}

				if (nDiff == 1)
				{
					if (cGroupTop->comparanda[iColDiff].sound)
					{
						bool isSomethingInGroup = false;
						for (itGroup.TryEnterGroup(); cGroup; cGroup = itGroup.Next())
						{
							if (cGroup->comparanda[iColDiff].formOrig)
							{
								isSomethingInGroup = true;
								break;
							}
							if (itGroup.IsEndOfGroup())
								break;
						}
						itGroup.TryExitGroup();


						if (!isSomethingInGroup)
							continue;


						if (!isDeviations)
						{
							isDeviations = true;

							nDeviationGroups++;

							nDeviations = L'A';
						}
						_ltow(nDeviationGroups, bufn, 10);

						lstrcat(bufn, L".");
						strcatb(bufn, nDeviations);

						lstrcat(bufn, L" ►►►");
						trOut->Add(bufn, IT_EMPTYLINEBEFORE | IT_LINEBRKAFTER, inMult);
						trOut->Add(NULL, IT_HORLINE, inMult);

						trOut->Add(L"Словарь", IT_COLUMN | IT_SPACE, inMult);
						trOut->Add(dictinfos[iColDiff].name, IT_LINEBRKAFTER, inMult);
						trOut->Add(NULL, IT_HORLINE, inMult);

						trOut->Add(L"Простейший ряд", IT_COLUMN | IT_TAB, inMult);
						OutputSoundsHeader(cGroupTop, trOut, inMult, false, false, IT_DASH, IT_LINEBRKAFTER);

						trOut->Add(NULL, IT_HORLINE, inMult);



						lstrcpy(bufn, L"График ");
						strcati(bufn, nDeviationGroups);
						lstrcat(bufn, L".");
						strcatb(bufn, nDeviations);
						trCld->Add(bufn, IT_COLUMN | IT_SPACE);//лучше разными вызовами Add и убрать к чёрту bufn

						trCld->Add(cGroupTop->comparanda[iColDiff].sound->Symbol, IT_SQRBRK | IT_SPACE);
						trCld->Add(L":", IT_SPACE);
						trCld->Add(cOther->comparanda[iColDiff].sound->Symbol, IT_SQRBRK | IT_LINEBRKAFTER);




						trCld->Add(cGroupTop->comparanda[iColDiff].sound->Symbol, IT_SQRBRK | IT_SPACE);
						trCld->Add(L"в простейшем ряду", IT_COLUMN | IT_SPACE);
						OutputSoundsHeader(cGroupTop, trCld, NULL, false, true, IT_DASH, IT_LINEBRKAFTER);

						nDeviations++;


						//единичные не берём

						{
							CognateList cl(trOut, inMult, trCld);

							for (itGroup.TryEnterGroup(); cGroup; cGroup = itGroup.Next())
							{
								if (cGroup->comparanda[iColDiff].formOrig)
								{
									OutputCognate(&cGroup->comparanda[iColDiff], trOut, inMult, true, IT_LINEBRK, &cl);
								}
								if (itGroup.IsEndOfGroup())
									break;
							}
							itGroup.TryExitGroup();

							trOut->Add(NULL, IT_HORLINE, inMult);

							if (cl.n > 1)
							{
								cl.Output();
								trOut->Add(NULL, IT_HORLINE, inMult);
							}
							else if (cl.n == 0)
								trCld->Add(NULL, IT_EMPTYLINEBEFORE);
						}





						trOut->Add(L"Отклонение", IT_COLUMN | IT_SPACE, inMult);
						trOut->Add(cGroupTop->comparanda[iColDiff].sound->Symbol, IT_SQRBRK, inMult);
						trOut->Add(L" : ", 0, inMult);
						trOut->Add(cOther->comparanda[iColDiff].sound->Symbol, IT_SQRBRK | IT_LINEBRKAFTER, inMult);

						trOut->Add(NULL, IT_LINEBRK, inMult);

						trOut->Add(L"Отклоняющийся ряд", IT_COLUMN | IT_TAB, inMult);
						cOther->dataExtra = (void*)1;
						OutputSoundsHeader(cOther, trOut, inMult, false, true, IT_DASH, IT_HORLINE);



						trCld->Add(cOther->comparanda[iColDiff].sound->Symbol, IT_SQRBRK | IT_SPACE);
						trCld->Add(L"в отклоняющемся ряду", IT_COLUMN | IT_SPACE);
						OutputSoundsHeader(cOther, trCld, NULL, false, true, IT_DASH, IT_LINEBRKAFTER);



						{
							bool isRows = false;
							CognateList cl(trOut, inMult, trCld);
							for (itOther.TryEnterGroup(); cOther; cOther = itOther.Next())
							{
								if (cOther->comparanda[iColDiff].formOrig)
								{
									OutputCognate(&cOther->comparanda[iColDiff], trOut, inMult, true, IT_LINEBRK, &cl);
								}
								if (!itOther.AreWeInsideGroup() || itOther.IsEndOfGroup())
									break;
							}


							trOut->Add(NULL, IT_HORLINE, inMult);
							if (cl.n > 1)
							{
								cl.Output();
								trOut->Add(NULL, IT_HORLINE, inMult);
							}
							if (cl.n == 0) trCld->Add(NULL, IT_EMPTYLINEBEFORE);

							OutputCognatesBySound(cGroupTop, cOther, iColDiff, trOut, inMult, trCld, cGroupTop);
							OutputCognatesBySound(cGroupTop, cOther, iColDiff, trOut, inMult, trCld, cOther);
						}

						trCld->Add(NULL, IT_SECTIONBRK);
					}
				}
			}
			if (isDeviations)
				trOut->Add(NULL, IT_HORLINE, inMult);

		}
		//		trOut->Add(NULL, IT_HORLINE, inMult);
		trOut->Add(NULL, IT_SECTIONBRK, inMult);
	}

	void OutputCorresponcesWithMaterial(Condition* cnd, InfoTree* trOut)
	{
		LPTSTR word;
		Sound* sound;

		InfoNode* inCnd, *inMult, *inOnce, *inMultList;

		inCnd = trOut->Add(cnd->title, IT_COLUMN | IT_LINEBRKBEFORE, NULL, false, cnd);
		inMultList = trOut->Add(L"Оглавление (только неединичные соответствия)", IT_COLUMN | IT_EMPTYLINEBEFORE | IT_LINEBRKAFTER, inCnd);
		inMult = trOut->Add(L"Материал — неединичные соответствия", IT_COLUMN | IT_EMPTYLINEBEFORE | IT_LINEBRKAFTER, inCnd);
		inOnce = trOut->Add(L"Материал — единичные соответствия", IT_COLUMN | IT_EMPTYLINEBEFORE | IT_LINEBRKAFTER, inCnd);

		OutputLanguageHeader(trOut, inMult);
		OutputLanguageHeader(trOut, inOnce);

		InfoNode* inTo = inOnce;

		Correspondence* c;
		for (CorrespondenceTree::Iterator it(&tCorrespondences); c = it.Next();)
		{
			if (it.IsStartOfGroup())
			{
				OutputSoundsHeader(c, trOut, inMultList, false, false, IT_DASH, IT_LINEBRK);

				inTo = inMult;
				OutputSoundsHeader(c, trOut, inTo, true, false, IT_TAB, IT_HORLINE);
			}


			for (int iCol = 0; iCol < nDicts; iCol++)
			{
				trOut->Add(c->comparanda[iCol].formOrig, IT_TAB, inTo);
				trOut->Add(c->comparanda[iCol].translation, IT_MARRQUOTES | IT_TAB, inTo);
			}

			if (it.IsEndOfGroup())
			{
				trOut->Add(NULL, IT_HORLINE, inTo);
				inTo = inOnce;
			}
		}

		trOut->Add(NULL, IT_HORLINE, inOnce);
		trOut->Add(NULL, IT_SECTIONBRK, inOnce);
	}

	void CalculateDistances(DistanceMatrix* mtx)
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
						mtx->GetDistance(iRow, iCol, c->comparanda[iRow].sound,
							c->comparanda[iRow].isSoundInCognates,
							c->comparanda[iCol].sound,
							c->comparanda[iCol].isSoundInCognates,
							true);
					}
				}
			}
			//if (it.IsEndOfGroup())
			//{
			//}
		}
	}

	void OutputDistances(Condition* cnd, DistanceMatrix* mtx, InfoTree* trOut)
	{
		LPTSTR word;
		Sound* sound;

		InfoNode* inCnd, *inMtx;

		inCnd = trOut->Add(cnd->title, IT_COLUMN | IT_LINEBRKBEFORE | IT_LINEBRKAFTER, NULL, false, cnd);
		trOut->Add(NULL, IT_LINEBRK, inCnd);
		trOut->Add(NULL, IT_LINEBRK, inCnd);

		trOut->Add(NULL, IT_TAB, inCnd);
		for (int iCol = 0; iCol < nDicts; iCol++)
			trOut->Add(dictinfos[iCol].name, IT_TAB, inCnd);
		inMtx = trOut->Add(NULL, IT_LINEBRK, inCnd);

		for (int iRow = 0; iRow < nDicts; iRow++)
		{
			trOut->Add(dictinfos[iRow].name, IT_TAB, inMtx);

			for (int iCol = 0; iCol < nDicts;/*iRow;*/ iCol++)
			{
				TCHAR bufn[20];
				trOut->Add(strcpyi(bufn, mtx->langs[iRow].dist[iCol]), IT_TAB, inMtx);
			}

			trOut->Add(NULL, IT_LINEBRK, inMtx);
		}

		trOut->Add(NULL, IT_HORLINE, inCnd);
		trOut->Add(NULL, IT_SECTIONBRK, inCnd);
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
};