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

	int GetDistance(int iRow, int iCol, Sound* s1, Sound* s2, bool doAdd = false)
	{
		int dist = 0;

		if (s1 && s2)
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


				//ВРЕМЕННАЯ МЕРА ДЛЯ НЕПРАВИЛЬНОЙ МФА В СЛОВАРЯХ
				if ((s1->Symbol[0] == L'x' && s2->Symbol[0] == L'χ') || (s2->Symbol[0] == L'x' && s1->Symbol[0] == L'χ'))
					dist = 0;
				if ((s1->Symbol[0] == L'w' && s2->Symbol[0] == L'v') || (s2->Symbol[0] == L'w' && s1->Symbol[0] == L'v'))
					dist = 0;
				////////////////////////////////////////////////
			}
		}


		if (doAdd)
			langs[iRow].dist[iCol] += dist;
		else
			langs[iRow].dist[iCol] = dist;
		return dist;
	}
};