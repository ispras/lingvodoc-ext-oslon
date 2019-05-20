﻿class Comparandum : public OwnNew
{
public:
	LPTSTR 		formIPA;
	LPTSTR 		formOrig;
	LPTSTR 		translation;
	LPTSTR		wLength, wF1, wF2, wF3;
	Sound*		sound;
	int			typeOfSegment;
	TCHAR		chrFragment[8];
	TCHAR		chrTranscr[8];
	bool		isSoundInCognates;
	bool		isSingleInGroup;

	Comparandum()
	{
		Reset_();
	}
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
		typeOfSegment == ST_NONE;
		chrFragment[0] = L'\0';
		isSoundInCognates = false;
		isSingleInGroup = false;
	}
	void SetFragment(LPTSTR text)
	{
		wcscpy(chrFragment, text);
		typeOfSegment = ST_FRAGMENT;
		isSoundInCognates = true;//надо переименовать
	}
	LPTSTR Text()
	{
		switch (typeOfSegment)
		{
		case ST_FRAGMENT:
			//ИСПРАВИТЬ!!! return chrFragment;
			return &chrFragment[0];
			//case ST_SOUND:
		default:
			//ИСПРАВИТЬ!!! return sound->Symbol;
			return &sound->Symbol[0];
			//case ST_NONE:
			//	return NULL;
			//default:
			//out(typeOfSegment);
			//	return L"!!";
		}
	}
	bool IsEqualTo(Comparandum* cmp2)
	{
		if (typeOfSegment != cmp2->typeOfSegment)
			return false;

		switch (typeOfSegment)
		{
		case ST_FRAGMENT:
			return !CompareFragmentWith(cmp2);
		case ST_SOUND:
			return sound == cmp2->sound;
		default:
			return false;
		}
	}
	char signed CompareFragmentWith(Comparandum* cmp2)
	{
		return wcscmp(chrFragment, cmp2->chrFragment);
	}
};


//////////////////////////////////////////////


class Correspondence : public BNode
{
public:
	int				iRow;
	//Correspondence*	crspNextSame;
	//Correspondence*	crspPrevSame;
	union
	{
		Correspondence*	last;
		Correspondence*	prev;
	};
	union
	{
		Correspondence*	first;
		Correspondence*	next;
	};
	Correspondence*	crspMain;

	Comparandum*	comparanda;
	int				rankAllSoundsSame;
	int				nSoundsSame;
	int				nSoundsEmpty;
	void*			dataExtra;
	int				iUnique;
	bool			isBeingChanged;

	Correspondence(int nDicts, int _iRow)
	{
		iRow = _iRow;

		comparanda = (Comparandum*)malloc(nDicts * sizeof(Comparandum));//new Comparandum[nDicts];
		memset(comparanda, '\0', nDicts * sizeof(Comparandum));

		_Reset();
	}

	void _Reset()
	{
		rankAllSoundsSame = 0;
		nSoundsSame = 0;
		nSoundsEmpty = 0;
		iUnique = 0;
		next = last = crspMain = NULL;
		isBeingChanged = false;
		dataExtra = NULL;
	}
	~Correspondence()
	{
		if (comparanda)
		{
			free(comparanda);//delete[] comparanda;
		}
	}
	void AddToGroup(Correspondence* _cMain)
	{
		crspMain = _cMain;

		Correspondence* _cPrev = crspMain->last;


		if (first)//т.е. это заголовок
		{
			crspMain->last = last;
			for (Correspondence* cNext = first; cNext; cNext = cNext->next)
				cNext->crspMain = crspMain;
		}
		else
			crspMain->last = this;


		prev = _cPrev;

		if (prev)
			prev->next = this;

		if (!crspMain->first)
			crspMain->first = this;
	}
	void RemoveFromGroup()
	{
		if (prev)
			prev->next = next;
		else
			crspMain->first = next;
		if (next)
			next->prev = prev;
		else
			crspMain->last = prev;

		next = prev = crspMain = NULL;
	}
};

//////////////////////////////////////////////////////////////

class CorrespondenceTree : public BTree
{
	int idUnique;
public:
	int nDicts;
	CorrespondenceTree(int _nDicts)
	{
		idUnique = 0;
		nDicts = _nDicts;
	}

	struct COMPAREFLAGS
	{
		bool skipEmpty;
		bool ignoreRankSame;
		bool ignoreUniqueID;
	};

	int CompareNodes(BNode* _nd1, BNode* _nd2, void* _struct)
	{
		COMPAREFLAGS* cf = (COMPAREFLAGS*)_struct;
		COMPAREFLAGS _cf;

		if (!cf)
		{
			_cf.skipEmpty = false;
			_cf.ignoreRankSame = false;
			_cf.ignoreUniqueID = false;
			cf = &_cf;
		}


		Correspondence* c1 = (Correspondence*)_nd1;//поэтому надо шаблонно!!
		Correspondence* c2 = (Correspondence*)_nd2;

		if (!cf->ignoreRankSame)
		{
			if (c1->rankAllSoundsSame > c2->rankAllSoundsSame)
				return -1;
			if (c1->rankAllSoundsSame < c2->rankAllSoundsSame)
				return 1;
		}

		bool isNonNull = false;
		//int nSame = 0;
		for (int iCol = 0; iCol < nDicts; iCol++)
		{
			Comparandum* cmp1 = &c1->comparanda[iCol];
			Comparandum* cmp2 = &c2->comparanda[iCol];
			int res;

			bool isCanCompare;
			if (cf->skipEmpty)
				isCanCompare = (cmp1->sound && cmp1->typeOfSegment != ST_EMPTYAUTOFILL && cmp2->sound && cmp2->typeOfSegment != ST_EMPTYAUTOFILL);
			else
				isCanCompare = true;

			if (isCanCompare)
			{
				isNonNull = true;
				if (cmp1->typeOfSegment == ST_FRAGMENT && cmp2->typeOfSegment == ST_FRAGMENT)
				{
					if (res = cmp1->CompareFragmentWith(cmp2))
						return res;
				}
				else if (cmp1->typeOfSegment == ST_FRAGMENT)
					return 1;
				else if (cmp2->typeOfSegment == ST_FRAGMENT)
					return -1;
				else if (res = CompareFeaturesAnd(cmp1->sound->feature, cmp2->sound->feature))
					return res;

				//nSame++;
			}
		}
		/*
				for (int iCol = 0; iCol < nDicts; iCol++)
				{
					int res;
					if (!cmp1->sound && cmp2->sound)
						return -1;
					if (cmp1->sound && !cmp2->sound)
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

		if (!cf->ignoreUniqueID)
		{
			if (c1->iUnique || c2->iUnique)
			{
				if (c1->iUnique > c2->iUnique)
					return 1;
				else if (c1->iUnique < c2->iUnique)
					return -1;
			}
		}

		return 0;
	}
	int GetUniqueID()
	{
		idUnique++;
		return idUnique;
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
				if (crspCurr->next)
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
				return (crspCurr->next);
			}
			else
			{
				return (crspCurrInGroup == Current());
			}
		}
		bool IsEndOfGroup()
		{
			if (!isInGroup) return false;

			return (crspCurrInGroup->next == NULL);
		}
		bool AreWeInsideGroup()
		{
			return isInGroup;
		}
		void NextAndCheck()
		{
			//TryAgain:
			if (!Walker::Next())
				return;

			Correspondence* crspCurr = (Correspondence*)Current();

			//if (crspCurr->degenerate)
			//	goto TryAgain;

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
			else if (!(crspCurrInGroup = crspCurrInGroup->next))
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
