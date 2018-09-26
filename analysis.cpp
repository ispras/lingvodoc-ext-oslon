//////////////////////////////////////////////////
//ПРЕДВАРИТЕЛЬНЫЙ ВАРИАНТ, ВЕРСИЯ -10.0
//////////////////////////////////////////////////

#include "unicode.h"
#include "strings.h"

LPTSTR ipaVowels = L"iɪɨɯeɛəɘɜɞaʌæɐäɒɑɵoɔøœɶuʉʏyʊɤ";
LPTSTR ipaConsonants = L"mɱnɳɲŋɴpbptdʈɖcɟkɡqɢʡʔszʃʒʂʐɕʑɸβfvθðɹçʝxɣχʁħʕhɦʋɹɻjɰⱱwɾɽʙrɽʀʜʢɬɮɭʎʟlɺ";
LPTSTR ipaModifiers = L"ʷʲ:ː̪̠̊̆ʼ̈";

enum
{
	CV_LABIAL,
	CV_DENTAL,
	CV_LIQUID,
	CV_LAST = CV_LIQUID,
};

LPTSTR CVbyGroup[CV_LAST + 1];
LPTSTR CVbyGroupName[CV_LAST + 1];

void __assigncvbygroup(int i, LPTSTR list, LPTSTR name) { CVbyGroup[i] = list; CVbyGroupName[i] = name; }

#define IT_COLUMN 			0x1
#define IT_LINEBRKBEFORE 	0x2
#define IT_LINEBRKAFTER 	0x4
#define IT_IDENT 			0x8
#define IT_COMMA 			0x10
#define IT_SQRBRK			0x20

class InfoNode
{
public:
	InfoNode* chldFirst;
	InfoNode* chldLast;
	InfoNode* next;

	LPTSTR	Title;
	LPTSTR	Data;

	int		Flags;

	InfoNode(LPTSTR title = NULL, LPTSTR data = NULL, int flags = IT_LINEBRKBEFORE)
	{
		next = chldFirst = chldLast = NULL;
		Title = StoreNonNullString(title);
		Data = StoreNonNullString(data);
		Flags = flags;
	}
	~InfoNode()
	{
		InfoNode* nd = chldFirst;
		while (nd)
		{
			InfoNode* ndNext = nd->next;
			delete nd;
			nd = ndNext;
		}
		FreeAndZero(Data);
		FreeAndZero(Title);
	}
};

class InfoTree
{
public:
	InfoNode* ndRoot;
	InfoTree()
	{
		ndRoot = NULL;
	}
	~InfoTree()
	{
		if (ndRoot)
			delete ndRoot;
	}
	InfoNode* Add(LPTSTR title = NULL, LPTSTR data = NULL, int flags = IT_COLUMN | IT_LINEBRKBEFORE, InfoNode* ndParent = NULL, bool isUnique = true)
	{
		if (!ndParent)
			ndParent = ndRoot;

		if (isUnique)
		{
			if (FindData(data, ndParent))
				return NULL;
		}

		InfoNode *nd = new InfoNode(title, data, flags);
		if (!ndParent->chldLast)
			ndParent->chldFirst = nd;
		else
			ndParent->chldLast->next = nd;
		ndParent->chldLast = nd;
		return nd;
	}

	InfoNode* FindData(LPTSTR data, InfoNode* ndParent = NULL)
	{
		if (!data)
			return NULL;

		//без рекурсии пока что
		if (!ndParent)
			ndParent = ndRoot;

		InfoNode* nd = ndParent->chldFirst;
		while (nd)
		{
			if (nd->Data)
			{
				if (!lstrcmp(data, nd->Data))
					return nd;
			}
			nd = nd->next;
		}
		return NULL;
	}
};


class Dictionary
{
public:
	class OutputString
	{
	public:
		TCHAR* bufOut;
		OutputString()
		{
			bufOut = new TCHAR[100000];
		}
		~OutputString()
		{
			delete[] bufOut;
		}
		LPTSTR Add(LPTSTR posOut, LPTSTR data, int flags, int iLevel, bool isFirst, bool isLast)
		{
			if (iLevel && (flags & IT_LINEBRKBEFORE))
			{
				lstrcpy(posOut, TEXT("\r\n"));
				posOut += 2;
			}
			if (flags & IT_IDENT)
			{
				for (int i = 0; i < iLevel; i++)
				{
					*posOut = '\t';
					posOut++;
				}
			}

			if (data)
			{
				if (flags & IT_SQRBRK)
				{
					lstrcpy(posOut, TEXT("["));
					posOut++;
				}

				int sz = lstrlen(data);
				if (sz)
				{
					lstrcpy(posOut, data);
					posOut += sz;
				}
				if (flags & IT_SQRBRK)
				{
					lstrcpy(posOut, TEXT("]"));
					posOut++;
				}

				if (flags & IT_COLUMN)
				{
					lstrcpy(posOut, TEXT(": "));
					posOut += 2;
				}

			}
			if ((flags & IT_COMMA) && !isLast && !isFirst)
			{
				lstrcpy(posOut, TEXT(", "));
				posOut += 2;
			}

			if (flags & IT_LINEBRKAFTER)
			{
				lstrcpy(posOut, TEXT("\r\n"));
				posOut += 2;
			}

			return posOut;
		}
		LPTSTR Build(InfoNode* ndCur, LPTSTR posOut = NULL, bool isFirst = true, int iLevel = 0)
		{
			if (iLevel == 0)
				posOut = bufOut;

			posOut = Add(posOut, ndCur->Title, ndCur->Flags, iLevel, isFirst, false);
			posOut = Add(posOut, ndCur->Data, 0, iLevel, isFirst, !ndCur->next);

			InfoNode* nd = ndCur->chldFirst;
			while (nd)
			{
				posOut = Build(nd, posOut, nd == ndCur->chldFirst, iLevel + 1);
				nd = nd->next;
			}

			if (iLevel == 0)
				posOut = bufOut;
			return posOut;
		}
	};

	void BuildWordList(InfoTree* trWordList, LPTSTR sIn)
	{
		trWordList->ndRoot = new InfoNode();
		InfoNode* ndWord;

		LPTSTR 	pos = sIn,
			end = sIn + lstrlen(sIn),
			old = pos;
		while (pos < end)
		{
			switch (*pos)
			{
			case '\r':
				*pos = 0;
				pos += 2;
				trWordList->Add(NULL, old);
				old = pos;
				break;
			default:
				pos++;
			}
		}
	}
	bool FindSymbolInWords(InfoTree* trWordList, LPTSTR symbol)
	{
		InfoNode* ndWord = trWordList->ndRoot->chldFirst;
		for (; ndWord; ndWord = ndWord->next)
		{
			LPTSTR posSmb = wcsstr(ndWord->Data, symbol);
			if (posSmb)
				return true;
		}
		return false;
	}

	void BuildSymbolList(InfoTree* trWordList, InfoTree* trOut, LPTSTR smbList, LPTSTR smbMod)
	{
		trOut->ndRoot = new InfoNode();

		for (LPTSTR pos = smbList; *pos; pos++)
		{
			TCHAR symbol[3];
			symbol[0] = *pos;
			symbol[1] = symbol[2] = '\0';

			if (FindSymbolInWords(trWordList, symbol))
				trOut->Add(NULL, symbol);

			for (LPTSTR posMod = smbMod; *posMod; posMod++)
			{
				symbol[1] = *posMod;
				if (FindSymbolInWords(trWordList, symbol))
					trOut->Add(NULL, symbol);
			}
		}
	}

	void BuildUnknownSymbolList(InfoTree* trWordList, InfoTree* trOut, LPTSTR smbAll)
	{
		trOut->ndRoot = new InfoNode();

		for (InfoNode* ndWord = trWordList->ndRoot->chldFirst; ndWord; ndWord = ndWord->next)
		{
			for (LPTSTR pos = ndWord->Data; *pos; pos++)
			{
				LPTSTR posSmbAll = wcschr(smbAll, *pos);
				if (!posSmbAll)
				{
					TCHAR symbol[2];
					symbol[0] = *pos;
					symbol[1] = '\0';
					trOut->Add(NULL, symbol);
				}
			}
		}
	}

	void GetDistributionLists(InfoNode* ndW, InfoTree* trOut, InfoTree& trSounds, InfoTree& trWordList)
	{
		for (InfoNode* nditS = trSounds.ndRoot->chldFirst; nditS; nditS = nditS->next)
		{
			InfoNode* ndThisSound = trOut->Add(nditS->Data, NULL, IT_IDENT | IT_SQRBRK | IT_COLUMN | IT_LINEBRKBEFORE, ndW);

			for (InfoNode* nditW = trWordList.ndRoot->chldFirst; nditW; nditW = nditW->next)
			{
				LPTSTR minposS = NULL;
				InfoNode* ndSFirst;
				for (InfoNode* nditSList = trSounds.ndRoot->chldFirst; nditSList; nditSList = nditSList->next)
				{
					LPTSTR posS = wcsstr(nditW->Data, nditSList->Data);
					if (posS)
					{
						if (!nditSList->Data[1] && wcschr(ipaModifiers, posS[1]))
							;
						else if (!minposS)
							goto FoundLeft;
						else if (posS < minposS)
						{
						FoundLeft:
							minposS = posS;
							ndSFirst = nditSList;
						}
					}
				}

				if (minposS)
				{
					if (ndSFirst == nditS)
					{
						trOut->Add(NULL, nditW->Data, IT_COMMA, ndThisSound);
					}
				}
			}

			InfoNode* ndAfter = trOut->Add(L"после", NULL, IT_COLUMN | IT_LINEBRKBEFORE | IT_IDENT, ndThisSound);
			InfoNode* ndBefore = trOut->Add(L"перед", NULL, IT_COLUMN | IT_LINEBRKBEFORE | IT_IDENT, ndThisSound);


			InfoNode* ndAfterGroups[CV_LAST + 1];
			for (int iGroup = 0; iGroup <= CV_LAST; iGroup++)
			{
				ndAfterGroups[iGroup] = trOut->Add(CVbyGroupName[iGroup], NULL, IT_COLUMN | IT_LINEBRKBEFORE | IT_IDENT, ndThisSound);
			}

			for (InfoNode* nditW = ndThisSound->chldFirst; nditW; nditW = nditW->next)
			{
				if (!nditW->Data)
					break;
				TCHAR symbol[3];

				LPTSTR posV = wcsstr(nditW->Data, ndThisSound->Title);

				symbol[1] = symbol[2] = '\0';

				if (posV == nditW->Data)
					symbol[0] = '#';
				else
				{
					if (!wcschr(ipaModifiers, posV[-1]))
						symbol[0] = posV[-1];
					else
					{
						symbol[0] = posV[-2];
						symbol[1] = posV[-1];
					}
				}

				trOut->Add(NULL, symbol, IT_COMMA, ndAfter);
				for (int iGroup = 0; iGroup <= CV_LAST; iGroup++)
				{
					LPTSTR posinGroup = wcschr(CVbyGroup[iGroup], symbol[0]);
					if (posinGroup && !ndAfterGroups[iGroup]->chldFirst)
						trOut->Add(L"+", NULL, 0, ndAfterGroups[iGroup]);
				}


				symbol[1] = symbol[2] = '\0';
				int szV = lstrlen(ndThisSound->Title);
				if (posV == nditW->Data + lstrlen(nditW->Data) - szV)
					symbol[0] = '#';
				else
				{
					symbol[0] = posV[szV];

					if (!wcschr(ipaModifiers, posV[szV + 1]))
						symbol[1] = '\0';
					else
						symbol[1] = posV[szV + 1];
				}
				trOut->Add(NULL, symbol, IT_COMMA, ndBefore);
			}

			bool isChld = false;
			for (int iGroup = 0; iGroup <= CV_LAST; iGroup++)
			{
				if (ndAfterGroups[iGroup]->chldFirst)
				{
					isChld = true;
					break;
				}
			}

			if (!isChld)
			{
				for (int iGroup = 0; iGroup <= CV_LAST; iGroup++)
				{
					delete ndAfterGroups[iGroup];
				}
				//ВРЕМЕННО
				ndBefore->next = NULL;
			}
		}
	}

	InfoNode* Analyze(InfoTree* trOut, LPTSTR sIn)
	{
		trOut->ndRoot = new InfoNode(L"ПРОСТЕЙШИЙ АНАЛИЗ НА ПРЕДМЕТ ПОИСКА ФОНЕМ", NULL, IT_LINEBRKAFTER);
		InfoNode* ndIter;

		int nLenIn;
		if (sIn) nLenIn = lstrlen(sIn);
		else	 nLenIn = 0;

		InfoTree trWordList, trVowels, trConsonants, trOutNotFound;

		TCHAR ipaAll[1000];
		lstrcpy(ipaAll, ipaVowels);
		lstrcat(ipaAll, ipaConsonants);
		lstrcat(ipaAll, ipaModifiers);

		BuildWordList(&trWordList, sIn);
		BuildSymbolList(&trWordList, &trVowels, ipaVowels, ipaModifiers);
		BuildSymbolList(&trWordList, &trConsonants, ipaConsonants, ipaModifiers);
		BuildUnknownSymbolList(&trWordList, &trOutNotFound, ipaAll);


		InfoNode* ndV = trOut->Add(L"Гласные звуки", NULL, IT_COLUMN | IT_LINEBRKBEFORE);
		for (ndIter = trVowels.ndRoot->chldFirst; ndIter; ndIter = ndIter->next)
			trOut->Add(NULL, ndIter->Data, IT_COMMA, ndV);

		InfoNode* ndC = trOut->Add(L"Согласные звуки", NULL, IT_COLUMN | IT_LINEBRKBEFORE);
		for (ndIter = trConsonants.ndRoot->chldFirst; ndIter; ndIter = ndIter->next)
			trOut->Add(NULL, ndIter->Data, IT_COMMA, ndC);

		InfoNode* ndNF = trOut->Add(L"Неопознанные знаки", NULL, IT_COLUMN | IT_LINEBRKBEFORE);
		for (ndIter = trOutNotFound.ndRoot->chldFirst; ndIter; ndIter = ndIter->next)
			trOut->Add(NULL, ndIter->Data, IT_COMMA, ndNF);


		InfoNode* ndW = trOut->Add(L"Списки по гласному первого слога", NULL, IT_COLUMN | IT_LINEBRKBEFORE);
		GetDistributionLists(ndW, trOut, trVowels, trWordList);

		ndW = trOut->Add(L"Списки по согласному первого слога", NULL, IT_COLUMN | IT_LINEBRKBEFORE);
		GetDistributionLists(ndW, trOut, trConsonants, trWordList);

		return trOut->ndRoot;
	}
};
///////////////////////////////////////////////////////////////////
extern "C" {
	int GetAllOutput(LPTSTR bufIn, LPTSTR bufOut)
	{
		InfoTree trOut;
		Dictionary dic;

		__assigncvbygroup(CV_LABIAL, L"pbvw", L"   после губных");
		__assigncvbygroup(CV_DENTAL, L"dtn", L"   после зубных");
		__assigncvbygroup(CV_LIQUID, L"lr", L"   после плавных");

		dic.Analyze(&trOut, bufIn);

		if (trOut.ndRoot)
		{
			Dictionary::OutputString output;
			output.Build(trOut.ndRoot);

			lstrcpy(bufOut, output.bufOut);
		}

		return 1;
	}
}
