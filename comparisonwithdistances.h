class ComparisonWithDistances : public Comparison
{
public:
	ComparisonWithDistances(int nRows, int nCols) : Comparison(nRows, nCols)
	{
	}

	void CalculateDistances(DistanceMatrix* mtx, int factor)
	{
		Correspondence* c;
		for (CorrespondenceTree::Iterator it(&tCorrespondences); c = it.Next();)
		{
			if (it.IsStartOfGroup())
			{
				for (int iRow = 0; iRow < nDicts; iRow++)
				{
					for (int iCol = 0; iCol < nDicts; iCol++)
					{
						mtx->GetDistance(iRow, iCol, &c->comparanda[iRow], &c->comparanda[iCol], factor, true);
					}
				}
			}
			//if (it.IsEndOfGroup())
			//{
			//}
		}
	}
	void RemoveDistancesIfTooFew(DistanceMatrix* mtx, int threshold)
	{
		for (int i = 0; i < nDicts; i++)
		{
			//if (dictinfos[i].nFilledSoundCorresp);

			//int percent;
			//if (nSoundCorresp)
			//	percent = (Dict(i)->dictinfo.nFilledSoundCorresp * 100) / nSoundCorresp;
			//else
			//	percent = 0;

			//if (percent < threshold)
			if (Dict(i)->dictinfo.nFilledSoundCorresp < threshold)
			{
				for (int ii = 0; ii < nDicts; ii++)
				{
					mtx->langs[i].dist[ii] = -1;
				}
			}
		}
	}
};