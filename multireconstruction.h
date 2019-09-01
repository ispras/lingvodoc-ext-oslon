class MultiReconstruction
{
public:
	int 			nGroups;
	Reconstruction* reconstructions;

	MultiReconstruction() {} //для массива
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
			for (int i = 0; i < rc->nCmp; i++)
				rc->cmp[i].Process(rc->cmp[i].condition, true, true);

			for (int i = 0; i < rc->nCmp; i++)
				rc->ReconstructSounds(i);

			rc->ReconstructWords();
		}
	}
};