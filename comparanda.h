class Comparandum : public OwnNew
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
	}
	void SetFragment(LPTSTR text)
	{
		wcscpy(chrFragment, text);
		typeOfSegment = ST_FRAGMENT;//но при этом sound = NULL, что ненормально
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
};

//////////////////////////////////////////////////////////////

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
			Comparandum* cmp1 = &c1->comparanda[iCol];
			Comparandum* cmp2 = &c2->comparanda[iCol];
			int res;
			if (cmp1->sound && cmp2->sound)
			{
				//if (!cmp1sound && c2->comparanda[iCol].sound)
				//	return 1;
				//if (cmp1 && !c2->comparanda[iCol].sound)
				//	return -1;

				isNonNull = true;

				if (cmp1->typeOfSegment == ST_FRAGMENT && cmp2->typeOfSegment == ST_FRAGMENT)
				{
					if (res = cmp1->CompareFragmentWith(cmp2))
						return res;
				}
				else if (cmp1->typeOfSegment == ST_FRAGMENT)
					return -1;
				else if (cmp2->typeOfSegment == ST_FRAGMENT)
					return 1;
				else if (res = CompareFeaturesAnd(cmp1->sound->feature, cmp2->sound->feature))
					return res;
			}
		}
		/*
				for (int iCol = 0; iCol < nDicts ; iCol++)
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
