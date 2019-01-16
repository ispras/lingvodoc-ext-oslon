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
				dist = mod(s1->RowNumber(FT_PLACE) - s2->RowNumber(FT_PLACE))
					+ mod(s1->RowNumber(FT_MANNER) - s2->RowNumber(FT_MANNER))
					+ mod(s1->RowNumber(FT_COARTICULATION) - s2->RowNumber(FT_COARTICULATION));
				//dist /= 2;
			}
		}


		if (doAdd)
			langs[iRow].dist[iCol] += dist;
		else
			langs[iRow].dist[iCol] = dist;
		return dist;
	}
};