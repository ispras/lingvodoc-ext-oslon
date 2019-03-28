class DistanceMatrix
{
public:
	class MtxLang
	{
	public:
		int* dist;
	};

	int nDicts;
	MtxLang* langs;

	DistanceMatrix(int _nDicts)
	{
		nDicts = _nDicts;

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

	int GetDistance(int iRow, int iCol, Sound* s1, bool isS1, Sound* s2, bool isS2, int factor, bool doAdd = false)
	{
		int dist = 0;

		if (s1 && isS1 && s2 && isS2)
		{
			if (s1 != s2)
			{
				//langs[iRow].dist[iCol]++;
				/*
				int* f1 = s1->feature;
				int* f2 = s2->feature;
				for (int iFType = 0; iFType < FT_NFEATURETYPES; iFType++)
				{
					int fMore, fLess;
					if (f1[iFType] > f2[iFType])
					{
						fMore = f1[iFType];
						fLess = f2[iFType];
					}
					else
					{
						fMore = f2[iFType];
						fLess = f1[iFType];
					}


					int diff = fMore - fLess;
					//dist + =f1[iFType] > f2[iFType])

					dist += diff;
				}
				*/

				dist = abs(s1->OrdinalInIPA(FT_PLACE) - s2->OrdinalInIPA(FT_PLACE))
					+ abs(s1->OrdinalInIPA(FT_MANNER) - s2->OrdinalInIPA(FT_MANNER))
					;//	 + abs(s1->OrdinalInIPA(FT_COARTICULATION) - s2->OrdinalInIPA(FT_COARTICULATION));


					//РАЗЛИЧИЯ СО ШВОЙ — РАССТОЯНИЕ 1!!!

				if (SpecDist(s1, s2, L'ə', &dist, 1));

				//ВРЕМЕННАЯ МЕРА ДЛЯ НЕПРАВИЛЬНОЙ МФА В СЛОВАРЯХ

				else if (SpecDist(s1, s2, L'x', L'χ', &dist, 0));
				else if (SpecDist(s2, s2, L'w', L'v', &dist, 0));
				else if (SpecDist(s1, s2, L'ʨ', L'ʧ', &dist, 0));

				else if (SpecDist(s1, s2, L'e', L'ɛ', &dist, 0));
				else if (SpecDist(s1, s2, L'o', L'ɔ', &dist, 0));
			}
		}


		dist *= factor;


		if (doAdd)
			langs[iRow].dist[iCol] += dist;
		else
			langs[iRow].dist[iCol] = dist;
		return dist;
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