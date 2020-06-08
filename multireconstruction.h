class MultiReconstruction
{
public:
	int 			nGroups;
	Reconstruction* reconstructions;

	MultiReconstruction(int* nCols, int _nGroups, int nRows, LPTSTR bufIn)
	{
		nGroups = _nGroups;
		reconstructions = new Reconstruction[nGroups];

		int begCols = 0, nColsAll = 0;

		for (int i = 0; i < nGroups; i++)
			nColsAll += nCols[i];

		for (int i = 0; i < nGroups; i++)
		{
			new (&reconstructions[i]) Reconstruction(nCols[i], nRows, bufIn, begCols, nColsAll);
			begCols += nCols[i];
		}
	}
	~MultiReconstruction()
	{
		delete[] reconstructions;
	}
	void ReconstructFirstLevel()
	{
		for (Reconstruction* rc = reconstructions; rc < reconstructions + nGroups; rc++)
		{
			for (int i = 0; i < rc->nComparisons; i++)
				//rc->comparisons[i].Process(rc->comparisons[i].condition, false, true);
				//rc->comparisons[i].Process(rc->comparisons[i].condition, true, true);
				rc->comparisons[i].Process(rc->comparisons[i].condition, true, false);

			for (int i = 0; i < rc->nComparisons; i++)
				rc->ReconstructSounds(i);

			rc->ReconstructWords();
		}
	}
};