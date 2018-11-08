#include "pool.h"

#define IT_COLUMN 			0x1
#define IT_LINEBRKBEFORE 	0x4
#define IT_LINEBRKAFTER 	0x8
#define IT_EMPTYLINEBEFORE 	0x10
#define IT_IDENT 			0x100
#define IT_COMMA 			0x200
#define IT_TAB	 			0x400
#define IT_SPACE	 		0x1000
#define IT_SQRBRK			0x2000

class InfoNode
{
public:
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
};

class InfoTree
{
public:
	Pool<TCHAR>	pString;
	InfoNode* 	ndRoot;

	InfoTree(LPTSTR title = NULL) : pString(500)
	{
		if (title)
			ndRoot = new InfoNode(title);//, NULL, IT_LINEBRKAFTER);
		else
			ndRoot = NULL;
	}
	~InfoTree()
	{
		if (ndRoot)
			delete ndRoot;
	}
	InfoNode* Add(LPTSTR data = NULL, int flags = IT_COLUMN | IT_LINEBRKBEFORE, InfoNode* ndParent = NULL, bool isUnique = false, void* dataExtra = NULL)
		//InfoNode* Add(LPTSTR title = NULL, LPTSTR data = NULL, int flags = IT_COLUMN|IT_LINEBRKBEFORE, InfoNode* ndParent = NULL, bool isUnique = false, void* dataExtra = NULL)
	{
		if (!ndRoot)
			ndRoot = new InfoNode(NULL, NULL, 0);

		if (!ndParent)
			ndParent = ndRoot;

		if (isUnique)
		{
			if (FindData(data, ndParent))
				return NULL;
		}

		//if (title)	title = pString.New(title, wcslen(title)+1);
		if (data)	data = pString.New(data, wcslen(data) + 1);

		InfoNode *nd = new InfoNode(data, flags, dataExtra);
		//InfoNode *nd = new InfoNode(title, data, flags, dataExtra);
		if (!ndParent->chldLast)
			ndParent->chldFirst = nd;
		else
			ndParent->chldLast->next = nd;
		ndParent->chldLast = nd;
		return nd;
	}

	void AddSubtree(InfoTree* trToAdd, LPTSTR title, int fMain = IT_COLUMN | IT_LINEBRKBEFORE, int fNodes = 0)
	{
		InfoNode* ndC = Add(title, fMain);
		//InfoNode* ndC = Add(title, NULL, fMain);
		if (trToAdd->ndRoot)
		{
			for (InfoNode* ndIter = trToAdd->ndRoot->chldFirst; ndIter; ndIter = ndIter->next)
				Add(ndIter->Data, ndIter->Flags | fNodes, ndC);
			//Add(NULL, ndIter->Data, ndIter->Flags|fNodes, ndC);		
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
		//if (iLevel && 
		if (flags & (IT_LINEBRKBEFORE | IT_EMPTYLINEBEFORE))
		{
			lstrcpy(posOut, TEXT("\r\n"));
			posOut += 2;
		}
		if (flags & IT_EMPTYLINEBEFORE)
		{
			lstrcpy(posOut, TEXT("\r\n"));
			posOut += 2;
		}
		if (flags & IT_IDENT)
		{
			for (int i = 0; i < iLevel; i++)
			{
				lstrcpy(posOut, TEXT("    "));
				posOut += 4;
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
		if (!isLast)// && !(flags & IT_LINEBRKBEFORE))
		{
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
				lstrcpy(posOut, TEXT("	"));
				posOut++;
			}
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

		posOut = Add(posOut, ndCur->Data, ndCur->Flags, iLevel, isFirst, !ndCur->next);
		//posOut = Add(posOut, ndCur->Data, 0, iLevel, isFirst, !ndCur->next);
		//posOut = Add(posOut, ndCur->Data, 0, iLevel, isFirst, !ndCur->next);

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
