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
		bool isS1 = cmp1->isSoundInCognates;
		bool isS2 = cmp2->isSoundInCognates;

		Segmentizer sgmntzr1(ipa, cmp1->chrFragment);
		Segmentizer sgmntzr2(ipa, cmp2->chrFragment);

		int distAll = 0;

		Sound* s1 = NULL;
		Sound* s2 = NULL;
		while (true)
		{
			if (cmp1->typeOfSegment == ST_FRAGMENT)
				s1 = sgmntzr1.GetNext();
			else if (!s1)
				s1 = cmp1->sound;
			else
				s1 = NULL;

			if (cmp2->typeOfSegment == ST_FRAGMENT)
				s2 = sgmntzr2.GetNext();
			else if (!s2)
				s2 = cmp2->sound;
			else
				s2 = NULL;

			if (!s1 && !s2)
				break;

			int dist = 0;

			bool isSameType = (cmp1->typeOfSegment == cmp2->typeOfSegment);
			bool isSound1 = !!s1 && isS1;//isS1 — входной, но лучше от него избавиться
			bool isSound2 = !!s2 && isS2;

			if (isSound1 && isSound2)
			{
				if (s1 != s2)
				{

					dist = abs(s1->OrdinalInIPA(FT_PLACE) - s2->OrdinalInIPA(FT_PLACE))
						+ abs(s1->OrdinalInIPA(FT_MANNER) - s2->OrdinalInIPA(FT_MANNER))
						;//	 + abs(s1->OrdinalInIPA(FT_COARTICULATION) - s2->OrdinalInIPA(FT_COARTICULATION));


						//РАЗЛИЧИЯ СО ШВОЙ — РАССТОЯНИЕ 1!!!

					if (SpecDist(s1, s2, L'ə', &dist, 1));

					//ВРЕМЕННАЯ МЕРА ДЛЯ НЕПРАВИЛЬНОЙ МФА В СЛОВАРЯХ

					else if (SpecDist(s1, s2, L'w', L'v', &dist, 0));
					else if (SpecDist(s1, s2, L'x', L'χ', &dist, 0));
					else if (SpecDist(s1, s2, L'x', L'h', &dist, 0));
					else if (SpecDist(s1, s2, L'χ', L'h', &dist, 0));
					else if (SpecDist(s1, s2, L'ʨ', L'ʧ', &dist, 0));

					else if (SpecDist(s1, s2, L'e', L'ɛ', &dist, 0));
					else if (SpecDist(s1, s2, L'o', L'ɔ', &dist, 0));

				}
			}
			else if (isSameType)//какой-то один есть или никакого нет
				dist = 0;
			else
				dist = 0; //пока оставляем так, но это сильно углубляет


			dist *= factor;

			distAll += dist;
		}

		if (doAdd)
			langs[iRow].dist[iCol] += distAll;
		else
			langs[iRow].dist[iCol] = distAll;



		return distAll;
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