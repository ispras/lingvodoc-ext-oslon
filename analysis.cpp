#include "unicode.h"
#include "strings.h"

//#include "python3.5m/Python.h"

/*
int main()
{
	
}
*/


LPTSTR ipaVowels = L"iɪɨɯeɛəɘɜɞaʌæɐäɒɑɵoɔøœɶuʉʏyʊɤ";
LPTSTR ipaConsonants = L"mɱnɳɲŋɴpbptdʈɖcɟkɡqɢʡʔszʃʒʂʐɕʑɸβfvθðɹçʝxɣχʁħʕhɦʋɹɻjɰⱱwɾɽʙrɽʀʜʢɬɮɭʎʟlɺ";
LPTSTR ipaModifiers = L"ʷʲ:ː̪̠̊̆ʼ̈";


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
				lstrcpy(posOut, L"\r\n");
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
					lstrcpy(posOut, L"[");
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
					lstrcpy(posOut, L"]");
					posOut++;
				}

				if (flags & IT_COLUMN)
				{
					lstrcpy(posOut, L": ");
					posOut += 2;
				}

			}
			if ((flags & IT_COMMA) && !isLast && !isFirst)
			{
				lstrcpy(posOut, L", ");
				posOut += 2;
			}

			if (flags & IT_LINEBRKAFTER)
			{
				lstrcpy(posOut, L"\r\n");
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
		while (true)
		{
			switch (*pos)
			{
			case '\r':
				*pos = 0;
				pos += 2;
			case '\0':
				trWordList->Add(NULL, old);
				old = pos;

				if (!*pos) return;
				break;
			default:
				pos++;
			}
		}
	}
	bool FindSymbolInWords(InfoTree* trWordList, LPTSTR symbol)
	{
		for (InfoNode* ndWord = trWordList->ndRoot->chldFirst; ndWord; ndWord = ndWord->next)
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

		for (InfoNode* nditV = trVowels.ndRoot->chldFirst; nditV; nditV = nditV->next)
		{
			InfoNode* ndThisVowel = trOut->Add(nditV->Data, NULL, IT_IDENT | IT_SQRBRK | IT_COLUMN | IT_LINEBRKBEFORE, ndW);

			for (InfoNode* nditW = trWordList.ndRoot->chldFirst; nditW; nditW = nditW->next)
			{
				LPTSTR minposV = NULL;
				InfoNode* ndVFirst;
				for (InfoNode* nditVList = trVowels.ndRoot->chldFirst; nditVList; nditVList = nditVList->next)
				{
					LPTSTR posV = wcsstr(nditW->Data, nditVList->Data);
					if (posV)
					{
						if (!nditVList->Data[1] && wcschr(ipaModifiers, posV[1]))
							;
						else if (!minposV)
							goto FoundLeft;
						else if (posV < minposV)
						{
						FoundLeft:
							minposV = posV;
							ndVFirst = nditVList;
						}
					}
				}

				if (minposV)
				{
					if (ndVFirst == nditV)
					{
						trOut->Add(NULL, nditW->Data, IT_COMMA, ndThisVowel);
					}
				}
			}
			InfoNode* ndAfter = trOut->Add(L"после", NULL, IT_COLUMN | IT_LINEBRKBEFORE | IT_IDENT, ndThisVowel);
			InfoNode* ndBefore = trOut->Add(L"перед", NULL, IT_COLUMN | IT_LINEBRKBEFORE | IT_IDENT, ndThisVowel);

			for (InfoNode* nditW = ndThisVowel->chldFirst; nditW; nditW = nditW->next)
			{
				if (!nditW->Data)
					break;
				TCHAR symbol[3];

				LPTSTR posV = wcsstr(nditW->Data, ndThisVowel->Title);

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


				symbol[1] = symbol[2] = '\0';
				int szV = lstrlen(ndThisVowel->Title);
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
		}
		return trOut->ndRoot;
	}
};
///////////////////////////////////////////////////////////////////
extern "C" {
	int GetAllOutput(LPTSTR bufIn, LPTSTR bufOut)
	{
		InfoTree trOut;
		Dictionary dic;

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
