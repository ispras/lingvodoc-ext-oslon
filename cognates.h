#pragma once
#include "datastructures.h"
#include "parser.h"
#include "ipa.h"

class Comparison
{
public:
	Dictionary 		dic;
	Pool<TCHAR>		pString;

	class Comparandum : public OwnNew
	{
	public:
		LPTSTR 		formIPA;
		LPTSTR 		formOrig;
		Sound*		sound;
		Comparandum(LPTSTR _formIPA, LPTSTR _formOrig)
		{
			formIPA = _formIPA;
			formOrig = _formOrig;
			sound = NULL;
		}
	};

	class Correspondence : public BNode
	{
	public:
		int				iRow;
		Correspondence*	crspNextSame;
		Comparandum*	comparanda;
		Correspondence(int nDicts, int _iRow)
		{
			iRow = _iRow;
			crspNextSame = NULL;
			comparanda = (Comparandum*)malloc(nDicts * sizeof(Comparandum));//new Comparandum[nDicts];
			//memset(comparanda, '\0', nDicts * sizeof(Comparandum));
			//for (int i = 0; i < nDicts; i++)
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

					//					//мошенничаем (временно)
					//					SoundTable::SoundTree* _st;
					//					if (res = _st->CompareNodes(c1->comparanda[iCol].sound, c2->comparanda[iCol].sound, NULL))
					//						return res;
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
	}
	~Comparison()
	{
		//delete[] corresps;
		for (int iRow = 1; iRow < nCorresp; iRow++)
		{
			if (corresps[iRow].comparanda)
			{
				free(corresps[iRow].comparanda);//corresps[iRow].~Correspondence();
				corresps[iRow].comparanda = NULL;
			}
		}

		free(corresps);
	}
	void Reset()
	{
		tCorrespondences.Empty();
		for (int iRow = 1; iRow < nCorresp; iRow++)
		{
			corresps[iRow].crspNextSame = NULL;
		}
	}
	int Process(Condition* cnd)
	{
		Reset();

		Segmentizer* sgmntzr = (Segmentizer*)malloc(nDicts * sizeof(Segmentizer));

		for (bool isAddingWithEmpty = false;; isAddingWithEmpty = true)
		{
			for (int iRow = 1; iRow < nCorresp; iRow++)
			{
				bool hasEmpty = false;
				for (int iCol = 0; iCol < nDicts; iCol++)
				{
					if (!corresps[iRow].comparanda[iCol].formIPA)
					{
						hasEmpty = true;
						break;
					}
				}

				if (isAddingWithEmpty == hasEmpty)
				{
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
								if (isYes)
									break;
								//else if (!isYes && QR_FIRSTINWORD)
								//	goto NextCorrespondence;
							}

							if (!isYes) goto NextCorrespondence;


							sound = sgmntzr[iCol].Current();
						}
						corresps[iRow].comparanda[iCol].sound = sound;
					}
					Correspondence* cFound = (Correspondence*)tCorrespondences.Add(&corresps[iRow]);
					if (cFound)
					{
						corresps[iRow].crspNextSame = cFound->crspNextSame;
						cFound->crspNextSame = &corresps[iRow];

						for (int iCol = 0; iCol < nDicts; iCol++)
						{
							if (!cFound->comparanda[iCol].sound)
								cFound->comparanda[iCol].sound = corresps[iRow].comparanda[iCol].sound;
						}
					}
				}
			NextCorrespondence:
				;
			}
			if (isAddingWithEmpty) break;
		}
		delete sgmntzr;
	}
	void OutputHeader(InfoTree* trOut, InfoNode* ndTo)
	{
		LPTSTR word;
		int fAdd = IT_TAB | IT_LINEBRKBEFORE;
		for (int iCol = 0; iCol < nDicts; iCol++)
		{
			if (iCol != 0) fAdd = IT_TAB;
			TCHAR buf[1000];
			_ltow(iCol + 1, buf, 10);
			word = buf;
			trOut->Add(word, fAdd, ndTo);
			//trOut->Add(NULL, word, fAdd, ndTo);
		}
	}
	void OutputLanguageNames(InfoTree* trOut)
	{
		TCHAR buf[500];
		int f = IT_EMPTYLINEBEFORE;
		for (int i = 0; i < nDicts; i++)
		{
			_ltow(i + 1, buf, 10);
			wcscat(buf, L" = ");
			wcscat(buf, corresps[0].comparanda[i].formOrig);
			trOut->Add(buf, IT_LINEBRKAFTER | f);
			//trOut->Add(NULL, buf, IT_LINEBRKAFTER|f);
			f = 0;
		}
	}
	void Output(Condition* cnd, InfoTree* trOut)
	{
		Correspondence* c;
		LPTSTR word;
		Sound* sound;

		int sz = nDicts * 20;
		LPTSTR bufHLine = new TCHAR[sz + 1];
		LPTSTR _line = L"—";
		for (int i = 0; i < sz; i++) bufHLine[i] = _line[0];
		bufHLine[sz] = '\0';


		InfoNode* inCnd, *inMult, *inOnce;

		inCnd = trOut->Add(cnd->title, /*NULL,*/ IT_COLUMN | IT_LINEBRKBEFORE, NULL, false, cnd);
		inMult = trOut->Add(L"Неединичные соответствия", /*NULL,*/ IT_COLUMN | IT_EMPTYLINEBEFORE, inCnd);
		inOnce = trOut->Add(L"Единичные соответствия", /*NULL,*/ IT_COLUMN | IT_EMPTYLINEBEFORE, inCnd);
		trOut->Add(/*NULL,*/bufHLine, IT_LINEBRKBEFORE, inMult);
		trOut->Add(/*NULL,*/bufHLine, IT_LINEBRKBEFORE, inOnce);


		InfoNode* inTo = inOnce;

		int fAdd;

		OutputHeader(trOut, inMult);
		OutputHeader(trOut, inOnce);



		for (CorrespondenceTree::Iterator it(&tCorrespondences); c = it.Next();)
		{
			if (it.IsStartOfGroup())
			{
				inTo = inMult;

				fAdd = IT_TAB | IT_LINEBRKBEFORE;

				trOut->Add(/*NULL,*/bufHLine, IT_LINEBRKBEFORE, inTo);
				for (int iCol = 0; iCol < nDicts; iCol++)
				{
					if (iCol != 0) fAdd = IT_TAB;

					if (!c->comparanda[iCol].sound)
						word = L"?";
					else
						word = c->comparanda[iCol].sound->Symbol;

					trOut->Add(word,/*NULL,*/fAdd | IT_SQRBRK, inTo);
				}
				trOut->Add(/*NULL,*/ bufHLine, IT_LINEBRKBEFORE, inTo);
			}

			fAdd = IT_TAB | IT_LINEBRKBEFORE;

			for (int iCol = 0; iCol < nDicts; iCol++)
			{
				if (iCol != 0) fAdd = IT_TAB;

				word = c->comparanda[iCol].formOrig;

				if (!word)
					word = L"—";

				trOut->Add(/*NULL,*/word, fAdd, inTo);
			}

			//TCHAR buf[10];
			//_ltow(c->iRow,buf,10);
			//trOut->Add(NULL, buf, fAdd);

			if (it.IsEndOfGroup())
			{
				trOut->Add(/*NULL,*/bufHLine, IT_LINEBRKBEFORE, inTo);
				inTo = inOnce;
			}
		}

		//trOut->Add(NULL, bufHLine, IT_LINEBRKBEFORE, inMult);
		trOut->Add(/*NULL,*/bufHLine, IT_LINEBRKBEFORE, inOnce);

		delete[] bufHLine;
	}
	void AddCognateList(LPTSTR sIn)
	{
		Parser parser(sIn, L"\0", PARSER_NONNULLEND);
		LPTSTR wordOrig, wordIPA;

		int iRow = -1, iCol = nDicts - 1;
		while (wordOrig = parser.Next())
		{
			if (dic.iReplacer == RT_NONE)
				dic.GuessReplacer(wordOrig);

			if (iCol == nDicts - 1)
			{
				iRow++;
				if (iRow >= nCorresp) break;
				iCol = 0;

			}
			else
			{
				//fAdd = IT_TAB; 
				iCol++;
			}

			TCHAR buf[1000];
			if (iRow == 0)
			{
				wordOrig = pString.New(wordOrig, wcslen(wordOrig) + 1);
				wordIPA = NULL;
			}
			else
			{
				if (!wordOrig[0])
					wordIPA = wordOrig = NULL;
				else
				{
					dic.ReplaceSymbols(wordOrig, buf);
					wordIPA = pString.New(buf, wcslen(buf) + 1);
					dic.ipa->SubmitWordForm(wordIPA);
				}
			}
			if (iCol == 0)
				new (corresps + iRow) Correspondence(nDicts, iRow);

			new (&corresps[iRow].comparanda[iCol]) Comparandum(wordIPA, wordOrig);
		}
	}
};