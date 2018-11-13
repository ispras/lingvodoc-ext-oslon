#pragma once
#include "datastructures.h"
#include "parser.h"
#include "ipa.h"

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
		Sound*		sound;
		bool		isSoundInCognates;
		Comparandum(LPTSTR _formIPA, LPTSTR _formOrig, LPTSTR _translation)
		{
			formIPA = _formIPA;
			formOrig = _formOrig;
			translation = _translation;
			sound = NULL;
			isSoundInCognates = false;
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

		Correspondence(int nDicts, int _iRow)
		{
			iRow = _iRow;
			crspNextSame = NULL;
			comparanda = (Comparandum*)malloc(nDicts * sizeof(Comparandum));//new Comparandum[nDicts];
			memset(comparanda, '\0', nDicts * sizeof(Comparandum));

			rankAllSoundsSame = 0;
			nSoundsSame = 0;
			nSoundsEmpty = 0;
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
			Correspondence*	crspCurrInGroup;
		public:
			Iterator(CorrespondenceTree* _tree) : Walker(_tree)
			{
				isInGroup = false;
				crspCurrInGroup = NULL;
			}

			bool IsStartOfGroup()
			{
				if (!isInGroup) return false;

				return (crspCurrInGroup == Current());
			}
			bool IsEndOfGroup()
			{
				if (!isInGroup) return false;

				return (crspCurrInGroup->crspNextSame == NULL);
			}
			void NextAndCheck()
			{
				if (!Walker::Next())
					return;

				Correspondence* crspCurr = (Correspondence*)Current();

				if (!isInGroup)
				{
					if (crspCurr->crspNextSame)
					{
						isInGroup = true;
						crspCurrInGroup = crspCurr;
					}
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
			}
			isProcessed = false;
		}
	}
	void AddCognateList(LPTSTR sIn)
	{
		Parser parser(sIn, L"\0", PARSER_NONNULLEND);
		LPTSTR wordOrig, wordIPA, wordTranslation;

		int iRow = -1, iCol = -1;
		while (parser.Next())
		{
			if (!dic.NextCol(iCol, iRow, nDicts, nCorresp)) break;

			if (iRow == -1)
				dic.GetDictInfo(parser, dictinfos[iCol]);
			else
			{
				dic.GetOrigIPAAndTranslation(parser, wordOrig, wordIPA, wordTranslation, dictinfos[iCol]);
				if (iCol == 0)
					new (&corresps[iRow]) Correspondence(nDicts, iRow);

				new (&corresps[iRow].comparanda[iCol]) Comparandum(wordIPA, wordOrig, wordTranslation);
			}
		}
	}
	int Process(Condition* cnd)
	{
		Reset();

		Segmentizer* sgmntzr = (Segmentizer*)malloc(nDicts * sizeof(Segmentizer));

		for (int i_nEmtpy = 0; i_nEmtpy <= nDicts; i_nEmtpy++)
		{
			for (int iRow = 0; iRow < nCorresp; iRow++)
			{
				bool nEmpty = 0;
				for (int iCol = 0; iCol < nDicts; iCol++)
				{
					if (!corresps[iRow].comparanda[iCol].formIPA)
						nEmpty++;
				}

				if (i_nEmtpy == nEmpty)
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
					//int rankAllSoundsSame = 0;

					for (int iCol = 0; iCol < nDicts; iCol++)
					{
						if (corresps[iRow].comparanda[iCol].formIPA)
						{
							if (!wasNotEmpty)
							{
								soundSameExact = soundSame = corresps[iRow].comparanda[iCol].sound;
								corresps[iRow].rankAllSoundsSame = 10;
								wasNotEmpty = true;
								corresps[iRow].nSoundsSame = 1;
							}
							else if (soundSame)
							{
								if (soundSame == corresps[iRow].comparanda[iCol].sound)
									corresps[iRow].nSoundsSame++;
								else
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
										corresps[iRow].nSoundsSame = 0;
									}
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

		isProcessed = true;
	}


	void OutputLanguageNames(InfoTree* trOut)
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
	void OutputHeader(InfoTree* trOut, InfoNode* ndTo)
	{
		LPTSTR word;
		//int fAdd = IT_TAB;
		trOut->Add(NULL, IT_HORLINE, ndTo);
		for (int iCol = 0; iCol < nDicts; iCol++)
		{
			//if (iCol != 0) fAdd = IT_TAB;
			//TCHAR buf[1000];
			//_ltow(iCol + 1, buf, 10);
			//word = buf;
			//trOut->Add(word, fAdd, ndTo);
			trOut->Add(dictinfos[iCol].name, IT_TAB, ndTo);
			trOut->Add(L"", IT_TAB, ndTo);
		}
		trOut->Add(NULL, IT_HORLINE, ndTo);
	}
	/*
		void OutputCorresponces(Condition* cnd, InfoTree* trOut)
		{
		}
	*/
	void OutputDeviationsWithMaterial(Condition* cnd, InfoTree* trOut)
	{
		InfoNode* inCnd, *inMult, *inOnce, *inMultList;

		inCnd = trOut->Add(cnd->title, IT_COLUMN | IT_LINEBRKBEFORE, NULL, false, cnd);
		inMultList = trOut->Add(L"Оглавление (отклонения только в одном соответствии)", IT_COLUMN | IT_EMPTYLINEBEFORE | IT_LINEBRKAFTER, inCnd);
		inMult = trOut->Add(L"Материал — отклонения", IT_COLUMN | IT_EMPTYLINEBEFORE | IT_LINEBRKAFTER, inCnd);
		OutputHeader(trOut, inMult);
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

		InfoNode* inTo = inOnce;

		int fAdd;

		OutputHeader(trOut, inMult);
		OutputHeader(trOut, inOnce);

		Correspondence* c;
		for (CorrespondenceTree::Iterator it(&tCorrespondences); c = it.Next();)
		{
			if (it.IsStartOfGroup())
			{

				for (int iCol = 0; iCol < nDicts; iCol++)
				{
					int fAdd = 0;
					if (iCol < nDicts - 1) fAdd |= IT_DASH;

					if (c->comparanda[iCol].isSoundInCognates)
					{
						fAdd |= IT_SQRBRK;
						word = c->comparanda[iCol].sound->Symbol;
					}
					else word = L" ? ";
					trOut->Add(word, fAdd, inMultList);
				}
				trOut->Add(NULL, IT_LINEBRK, inMultList);



				inTo = inMult;

				for (int iCol = 0; iCol < nDicts; iCol++)
				{
					int fAdd;

					if (c->comparanda[iCol].isSoundInCognates)
					{
						fAdd = IT_SQRBRK;
						word = c->comparanda[iCol].sound->Symbol;
					}
					else
					{
						fAdd = 0;
						word = L" ? ";
					}

					trOut->Add(word, IT_TAB | fAdd, inTo);
					trOut->Add(L"", IT_TAB, inTo);
				}
				trOut->Add(NULL, IT_HORLINE, inTo);
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
		trOut->Add(NULL, IT_SECTIONBRK, inTo);
	}
};