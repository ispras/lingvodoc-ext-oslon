class DistanceMatrix
{
public:
	class MtxLang
	{
	public:
		int* dist;
	};

	IPA *ipa;
	int nDicts;
	MtxLang* langs;

	DistanceMatrix(IPA* _ipa, int _nDicts) //: ipa(_ipa), nDicts(_nDicts)
	{
		ipa = _ipa; nDicts = _nDicts;

		langs = new MtxLang[nDicts];

		for (int i = 0; i < nDicts; i++)
		{
			langs[i].dist = new int[nDicts];
			for (int ii = 0; ii < nDicts; ii++)
				langs[i].dist[ii] = 0;
		}
	}
	~DistanceMatrix()
	{
		for (int i = 0; i < nDicts; i++)
		{
			delete[] langs[i].dist;
		}
		delete langs;
	}
	/*
		int GetDistance(int iRow, int iCol, Comparandum* cmp1, Comparandum* cmp2, int factor, bool doAdd = false)
		{
			return GetDistance(iRow, iCol, cmp1->sound, cmp1->isSoundInCognates, cmp2->sound, cmp2->isSoundInCognates, factor, doAdd);

			Sound* s1 = cmp1->sound;
			Sound* s2 = cmp2->sound;

			bool isS1 = cmp1->isSoundInCognates;
			bool isS2 = cmp2->isSoundInCognates;
		}
	*/
	//	int GetDistance(int iRow, int iCol, Sound* s1, bool isS1, Sound* s2, bool isS2, int factor, bool doAdd = false)
	int GetDistance(int iRow, int iCol, Comparandum* cmp1, Comparandum* cmp2, int factor, bool doAdd = false)
	{
		int distAll = 0;

		bool isS1 = cmp1->isSoundInCognates;
		bool isS2 = cmp2->isSoundInCognates;

		Segmentizer sgmntzr1(ipa, cmp1->sg.chrFragment);
		Segmentizer sgmntzr2(ipa, cmp2->sg.chrFragment);

		Sound* s1 = NULL;
		Sound* s2 = NULL;
		while (true)
		{
			if (cmp1->sg.typeOfSegment == ST_FRAGMENT)			s1 = sgmntzr1.GetNext();
			else if (!s1)										s1 = cmp1->sg.sound;
			else												s1 = NULL;

			if (cmp2->sg.typeOfSegment == ST_FRAGMENT)			s2 = sgmntzr2.GetNext();
			else if (!s2) 										s2 = cmp2->sg.sound;
			else												s2 = NULL;

			if (!s1 && !s2)
				break;

			int dist = 0;

			bool isNotSameType = (cmp1->sg.typeOfSegment == ST_SOUND && cmp2->sg.typeOfSegment == ST_FRAGMENT)
				|| (cmp2->sg.typeOfSegment == ST_SOUND && cmp1->sg.typeOfSegment == ST_FRAGMENT);
			bool isSound1 = !!s1 && isS1;//isS1 — входной, но лучше от него избавиться
			bool isSound2 = !!s2 && isS2;

			if (isSound1 && isSound2)
			{
				if (s1 != s2)
				{

					//ВРЕМЕННАЯ МЕРА ДЛЯ НЕПРАВИЛЬНОЙ МФА В СЛОВАРЯХ

					if (!(Replace(&s1, &s2, L'w', L'v')))
						if (!(Replace(&s1, &s2, L'χ', L'x')))
							if (!(Replace(&s1, &s2, L'h', L'x')))
								if (!(Replace(&s1, &s2, L'ʥ', L'ʤ')))
									if (!(Replace(&s1, &s2, L'ʨ', L'ʧ')))
										if (!(Replace(&s1, &s2, L'ɛ', L'e')))
											if (!(Replace(&s1, &s2, L'ɔ', L'o')));


					dist = abs(s1->OrdinalInIPA(FT_PLACE) - s2->OrdinalInIPA(FT_PLACE))
						+ abs(s1->OrdinalInIPA(FT_MANNER) - s2->OrdinalInIPA(FT_MANNER))
						;//	 + abs(s1->OrdinalInIPA(FT_COARTICULATION) - s2->OrdinalInIPA(FT_COARTICULATION));


						//РАЗЛИЧИЯ СО ШВОЙ — РАССТОЯНИЕ 1!!!

					if (SpecDist(s1, s2, L'ə', &dist, 1));

					//ВРЕМЕННАЯ МЕРА ДЛЯ НЕПРАВИЛЬНОЙ МФА В СЛОВАРЯХ
					/*
					else if	(SpecDist(s1, s2, L'w', L'v', &dist, 0));
					else if	(SpecDist(s1, s2, L'x', L'χ', &dist, 0));
					else if	(SpecDist(s1, s2, L'x', L'h', &dist, 0));
					else if	(SpecDist(s1, s2, L'χ', L'h', &dist, 0));
					else if	(SpecDist(s1, s2, L'ʨ', L'ʧ', &dist, 0));

					else if	(SpecDist(s1, s2, L'e', L'ɛ', &dist, 0));
					else if	(SpecDist(s1, s2, L'o', L'ɔ', &dist, 0));
					*/
				}
			}
			else if (isNotSameType)
				dist = 0;
			else if ((cmp1->sg.typeOfSegment == ST_NULL && (cmp2->sg.typeOfSegment == ST_SOUND || cmp2->sg.typeOfSegment == ST_FRAGMENT))
				|| (cmp2->sg.typeOfSegment == ST_NULL && (cmp1->sg.typeOfSegment == ST_SOUND || cmp1->sg.typeOfSegment == ST_FRAGMENT)))
				dist = 4;
			else
				dist = 0;


			dist *= factor;

			distAll += dist;
		}

		if (doAdd)
			langs[iRow].dist[iCol] += distAll;
		else
			langs[iRow].dist[iCol] = distAll;

		return distAll;
	}
	bool Replace(Sound** s1, Sound** s2, TCHAR chFrom, TCHAR chTo)
	{
		bool ret = false;
		if ((*s1)->Symbol[0] == chFrom)
		{
			*s1 = ipa->GetSound(chTo);
			ret = true;
		}
		if ((*s2)->Symbol[0] == chFrom)
		{
			*s2 = ipa->GetSound(chTo);
			ret = true;
		}
		return ret;
	}
	bool SpecDist(Sound* s1, Sound* s2, TCHAR ch, int* dist, int distNew)
	{
		if ((s1->Symbol[0] == ch) || (s2->Symbol[0] == ch))
		{
			*dist = distNew;
			return true;
		}
		return false;
	}
	bool SpecDist(Sound* s1, Sound* s2, TCHAR ch1, TCHAR ch2, int* dist, int distNew)
	{
		if ((s1->Symbol[0] == ch1 && s2->Symbol[0] == ch2) || (s2->Symbol[0] == ch1 && s1->Symbol[0] == ch2))
		{
			*dist = distNew;
			return true;
		}
		return false;
	}
};

class Distance : public BNode
{
public:
	Distance(Sound* sd)
	{
		cmp.sg.SetSound(sd);
		cmp.isSoundInCognates = true;
		dist = 0;
	}

	Comparandum	cmp;
	int				dist;
};

class DistanceTree : public BTree
{
	Pool<Distance>		pDistance;
public:
	DistanceTree() : pDistance(1000)
	{
	}
	Distance* New(Sound* sd)
	{
		Distance* d = new (pDistance.New()) Distance(sd);
		return d;
	}
	int CompareNodes(BNode* _nd1, BNode* _nd2, void* _struct)
	{

		Distance* d1 = (Distance*)_nd1;//поэтому надо шаблонно!!
		Distance* d2 = (Distance*)_nd2;

		if (d1->dist < d2->dist)
			return -1;
		else
			return 1;

		return 0;
	}
};
