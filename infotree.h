#include "pool.h"

#define IT_COLUMN 			0x1
#define IT_LINEBRKBEFORE 	0x4
#define IT_LINEBRKAFTER 	0x8
#define IT_EMPTYLINEBEFORE 	0x10
//#define IT_HORLINEBEFORE 	0x20
#define IT_HORLINEAFTER 	0x40

#define IT_LINEBRK		 	0x100
#define IT_HORLINE		 	0x200
#define IT_SECTIONBRK	 	0x400

#define IT_IDENT 			0x1000
#define IT_COMMA 			0x2000
#define IT_TAB	 			0x4000
#define IT_DASH	 			0x8000
#define IT_COLUMNWITHSPACES	0x10000
#define IT_SPACE	 		0x100000
#define IT_SQRBRK			0x200000
#define IT_MARRQUOTES		0x400000

class InfoNode
{
public:
	void* operator new (size_t size, void* memAlloc) { return memAlloc; }

	InfoNode* chldFirst;
	InfoNode* chldLast;
	InfoNode* next;

	//	LPTSTR	Title;
	LPTSTR	Data;

	int		Flags;
	void*	dataExtra;

	//	InfoNode(LPTSTR title = NULL, LPTSTR data = NULL, int flags = 0, void* _dataExtra = NULL)
	InfoNode(LPTSTR data = NULL, int flags = 0, void* _dataExtra = NULL)
	{
		next = chldFirst = chldLast = NULL;
		//Title = title;//StoreString(title);//пока у нас никаких слеек нет, можно не заводить

		//Title = title;
		Data = data;

		//Data = data;//StoreString(data);
		Flags = flags;
		dataExtra = _dataExtra;
	}
	//~InfoNode()
	//{
	//	FreeAndZero(&Data);
	//	FreeAndZero(&Title);
	//}
/*
	void DeleteChildren()
	{
		DeleteChildren();
		InfoNode* nd = chldFirst;
		while (nd)
		{
			InfoNode* ndNext = nd->next;
			delete nd;
			nd = ndNext;
		}
		chldFirst = chldLast = NULL;
	}
*/
};

class InfoTree
{
public:
	Pool<TCHAR>		pString;
	Pool<InfoNode>	pNodes;
	InfoNode* 		ndRoot;

	InfoTree(LPTSTR title = NULL) : pString(500), pNodes(1000)
	{
		if (title)
			ndRoot = new (pNodes.New()) InfoNode(title, 0);
		else
			ndRoot = NULL;
	}
	/*
		~InfoTree()
		{
			if (ndRoot)
				delete ndRoot;
		}
	*/
	InfoNode* Add(LPTSTR data = NULL, int flags = IT_COLUMN | IT_LINEBRKBEFORE, InfoNode* ndParent = NULL, bool isUnique = false, void* dataExtra = NULL)
	{
		if (!ndRoot)
			ndRoot = new (pNodes.New()) InfoNode(NULL, 0, NULL);

		if (!ndParent)
			ndParent = ndRoot;

		if (isUnique)
		{
			if (FindData(data, ndParent))
				return NULL;
		}

		if (data)	data = pString.New(data, wcslen(data) + 1);

		InfoNode *nd = new (pNodes.New()) InfoNode(data, flags, dataExtra);
		//InfoNode *nd = new InfoNode(data, flags, dataExtra);

		if (!ndParent->chldLast)
			ndParent->chldFirst = nd;
		else
			ndParent->chldLast->next = nd;
		ndParent->chldLast = nd;
		return nd;
	}
	void HorLine()
	{
		Add(NULL, IT_HORLINE);
	}
	void AddSubtree(InfoTree* trToAdd, LPTSTR title, int fMain = IT_COLUMN | IT_LINEBRKBEFORE, int fNodes = 0)
	{
		InfoNode* ndC = Add(title, fMain);
		if (trToAdd->ndRoot)
		{
			for (InfoNode* ndIter = trToAdd->ndRoot->chldFirst; ndIter; ndIter = ndIter->next)
				Add(ndIter->Data, ndIter->Flags | fNodes, ndC);
		}
	}
	InfoNode* FindData(LPTSTR data, InfoNode* ndParent = NULL)
	{
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


class OutputString
{
public:
	TCHAR* 	bufOut;
	TCHAR* 	posOut;
	TCHAR* 	posOutOld;
	TCHAR* 	bufHLine;
	bool	wasLnBrk;
	bool	isBinary;
	int		iColCur;
	int		szHLine;
	int		OutputSize;
	int		nCols;
	int		nRows;
	int		wCol;
	int		nSections;
	int 	szMax;

	class Section
	{
	public:
		void* operator new (size_t size, void* memAlloc) { return memAlloc; }
		int nCols;
		int nRows;
		Section(int _nCols, int _nRows)
		{
			nCols = _nCols;
			nRows = _nRows;
		}
	};
	Pool<Section>	pSections;

	OutputString(int _szMax, int _wCol = 20, int _nCols = 0, bool _isBinary = false) : pSections(5)
	{
		szMax = _szMax;
		bufOut = new TCHAR[szMax + 20];

		bufHLine = NULL;
		szHLine = 0;

		nCols = _nCols;
		wCol = _wCol;
		iColCur = 0;
		wasLnBrk = true;
		nSections = 0;
		nRows = 0;
		OutputSize = 0;

		isBinary = _isBinary;
	}
	~OutputString()
	{
		if (bufOut)
			delete[] bufOut;
		if (bufHLine)
			delete[] bufHLine;
	}

	int CheckSize(LPTSTR data, int flags)
	{
		int szToAdd = 0;
		if (flags & IT_HORLINE)
		{
			GetHorLine();
			szToAdd += szHLine + 4;
		}
		if (flags & IT_LINEBRK)
		{
			szToAdd += nCols + 2;
		}
		if (data)
			szToAdd += lstrlen(data);
		if (posOut + szToAdd + 20 - bufOut >= szMax)
			return -1;
		else
			return szToAdd;
	}
	LPTSTR GetHorLine()
	{
		if (!bufHLine)
		{
			if (nCols == 0)
				szHLine = wCol;
			else
				szHLine = nCols * wCol;
			bufHLine = new TCHAR[szHLine + 1];
			for (int i = 0; i < szHLine; i++) bufHLine[i] = L'—';
			bufHLine[szHLine] = L'\0';
		}
		return bufHLine;
	}
	void LineBreak(bool noDouble = false)
	{
		if (noDouble && wasLnBrk)
			return;

		if (isBinary)
		{
			while (iColCur < nCols)
				Tab();

			posOut[0] = L'\0';
			posOut += 1;
		}
		else
		{
			lstrcpy(posOut, TEXT("\r\n"));
			posOut += 2;
		}
		iColCur = 0;
		nRows++;
	}
	void Tab()
	{
		if (isBinary)
			posOut[0] = L'\0';
		else
			lstrcpy(posOut, TEXT("	"));
		posOut++;

		iColCur++;
	}
	void SectionBreak()
	{
		if (isBinary)
		{
			LineBreak(true);

			Section* sect = new (pSections.New()) Section(nCols, nRows);
			nSections++;
		}
		else
		{
			lstrcpy(posOut, L"***");
			posOut += 3;
			LineBreak();
		}
		nRows = 0;
	}
	void Add(LPTSTR data, int sz, int flags, int iLevel, bool isFirst, bool isLast)
	{
		posOutOld = posOut;
		bool isLnBrk = wasLnBrk;

		if (flags & IT_SECTIONBRK)//с др. флагами не должен совмещаться
		{
			SectionBreak();
			isLnBrk = true;
		}

		if (flags & IT_LINEBRK)//с др. флагами не должен совмещаться
		{
			LineBreak(true);
			isLnBrk = true;
		}
		if (flags & IT_HORLINE)
		{
			if (!isBinary)
			{
				LineBreak(true);
				lstrcpy(posOut, GetHorLine());
				posOut += szHLine;
			}
			LineBreak();
			isLnBrk = true;
		}

		if (flags & (IT_LINEBRKBEFORE | IT_EMPTYLINEBEFORE))
		{
			LineBreak(true);
		}
		if (flags & IT_EMPTYLINEBEFORE)
		{
			LineBreak();
		}
		if (flags & IT_IDENT)
		{
			for (int i = 0; i < iLevel; i++)
			{
				lstrcpy(posOut, TEXT("    "));
				posOut += 4;
			}
		}

		if (nCols && iColCur >= nCols)
		{
			LineBreak(true);
		}

		if (data)
		{
			isLnBrk = false;
			if (flags & IT_SQRBRK)
			{
				lstrcpy(posOut, TEXT("["));
				posOut++;
			}
			if (flags & IT_MARRQUOTES)
			{
				lstrcpy(posOut, TEXT("ʽ"));
				posOut++;
			}

			if (sz)
			{
				lstrcpy(posOut, data);
				posOut += sz;

			}
			if (flags & IT_MARRQUOTES)
			{
				lstrcpy(posOut, TEXT("ʼ"));
				posOut++;
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
		if (!isLast)// && !(flags & IT_LINEBRKBEFORE))
		{
			isLnBrk = false;
			if (flags & IT_COMMA)
			{
				lstrcpy(posOut, TEXT(","));
				posOut++;
			}
			if (flags & IT_SPACE)
			{
				lstrcpy(posOut, TEXT(" "));
				posOut++;
			}
			if (flags & IT_TAB)
			{
				if (!isBinary && nCols > 0)
				{
					if (posOut - posOutOld >= wCol)		//if (nCols && (posOut -  posOutOld > wCol))//так не прёт
					{
						sz = wCol - 1;
						posOutOld[sz - 1] = L'…';
						posOutOld[sz] = L'\0';
						posOut = posOutOld + sz;
					}
				}

				Tab();
			}
			if (flags & IT_COLUMNWITHSPACES)
			{
				lstrcpy(posOut, TEXT(" : "));
				posOut += 3;
			}
			if (flags & IT_DASH)
			{
				lstrcpy(posOut, TEXT("—"));
				posOut++;
			}
		}

		if (flags & (IT_LINEBRKAFTER | IT_HORLINEAFTER))
		{
			LineBreak();
			isLnBrk = true;
		}

		wasLnBrk = isLnBrk;
	}
	bool Build(InfoNode* ndCur, bool isFirst = true, int iLevel = 0)
	{
		if (iLevel == 0)
		{
			OutputSize = 0;
			posOut = bufOut;
		}

		int sz = CheckSize(ndCur->Data, ndCur->Flags);
		if (sz == -1)
		{
			lstrcpy(posOut, TEXT("!НЕ ВЛЕЗЛО!"));
			posOut += 12;//потом нормально сделаю
			return false;
		}

		Add(ndCur->Data, sz, ndCur->Flags, iLevel, isFirst, !ndCur->next);

		InfoNode* nd = ndCur->chldFirst;
		while (nd)
		{
			if (!(Build(nd, nd == ndCur->chldFirst, iLevel + 1)))
				return false;
			nd = nd->next;
		}

		//	if (iLevel == 0)
		//	{
		//		posOut = bufOut;
		//	}
		//	return posOut;
		return true;
	}
	void OutputTableSizes(LPTSTR bufExport)
	{
		bufExport[0] = L'\0';

		TCHAR bufNum[20];
		for (int i = 0; i < nSections; i++)
		{
			OutputString::Section* s = pSections.GetByNum(i);

			_ltow(s->nCols, bufNum, 10);
			lstrcat(bufExport, bufNum);
			lstrcat(bufExport, L",");

			_ltow(s->nRows, bufNum, 10);
			lstrcat(bufExport, bufNum);
			if (i < nSections - 1)
				lstrcat(bufExport, L",");
		}

		OutputSize += lstrlen(bufExport);
		if (OutputSize) OutputSize += 1;
	}
	void OutputData(LPTSTR bufExport)
	{
		memcpy(bufExport + OutputSize, bufOut, ((posOut - bufOut) + 1) * sizeof(TCHAR));
		OutputSize += (posOut - bufOut);
	}
};
