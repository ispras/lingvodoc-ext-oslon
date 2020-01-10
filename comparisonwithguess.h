class ComparisonWithGuess : public Comparison
{
	//	int 	nRowsCorresp;
	//	int		nRowsNoCorresp;
public:
	ComparisonWithGuess(int _nRowsCorresp, int nCols, int _nRowsNoCorresp) : Comparison(_nRowsCorresp, nCols, _nRowsNoCorresp)
	{
		//	nRowsAll = 0;
	}

	void ProcessAndOutput(InfoTree* trOut, Condition* cndMatch, int iDictThis, bool doLookMeaning)
	{
		TCHAR buf[1000];

		int const maxnWF = 100;

		WordForm*** wfsOrphans = new WordForm**[nDicts];
		int* nsOrphans = new int[nDicts];
		for (int i = 0; i < nDicts; i++) wfsOrphans[i] = new WordForm*[maxnWF];

		//for (int iDictThis = 0; iDictThis < nDicts; iDictThis++)
		//{
		Dictionary* dic = Dict(iDictThis);

		_ltow(iDictThis + 1, buf, 10); wcscat(buf, L": ");
		if (dic->dictinfo.name) wcscat(buf, dic->dictinfo.name);
		trOut->Add(buf, IT_LINEBRKAFTER);

		trOut->HorLine();

		WordForm* wordThis;
		for (BTree::Walker w(&dic->trWordForms); wordThis = (WordForm*)w.Next();)
		{
			if (wordThis->flags & WF_HASLINK) continue;

			trOut->Add(L"Предложения для", IT_SPACE);
			trOut->Add(wordThis->formOrig, IT_SPACE);
			trOut->Add(wordThis->wordTranslation, IT_MARRQUOTES | IT_COLUMN);
			//trOut->HorLine();

			Comparandum cmpThis(wordThis);
			cmpThis.typeOfSegment = cndMatch->GetFirstMatchingFragment(
				dic->ipa,
				&cmpThis.sound,
				(cmpThis.wf ? cmpThis.wf->formIPA : NULL),
				cmpThis.chrFragment);

			for (int i = 0; i < nDicts; i++)
				nsOrphans[i] = 0;
			bool isMatchingRows = false;
			bool nMatchingOrphanRows = 0;

			if (nRowsNotOrphan < 10)//ПРОИЗВОЛЬНО!
			{
				nMatchingOrphanRows = LookForMatchingOrphans(wordThis, cndMatch, &cmpThis, NULL, wfsOrphans, nsOrphans, true, nMatchingOrphanRows, maxnWF);
			}
			else
			{
				Correspondence* corr;
				for (CorrespondenceTree::Iterator it(&tCorrespondences); corr = it.Next();)
				{
					Comparandum* cmp = &corr->comparanda[iDictThis];

					//if (cmp->wf == wordThis) continue;
					if (corr->nSoundsEmpty == nDicts - 1)//значит сирота, в т.ч. wordThis
						continue;

					switch (cmp->typeOfSegment)
					{
					case ST_FRAGMENT:
						if (!cmpThis.IsEqualTo(cmp)) continue;
						break;
					case ST_EMPTYAUTOFILL:
					case ST_NONE:
						bool isSomeOtherColMatching = false;
						for (int iCol = 0; iCol < nDicts; iCol++)
						{
							if (isSomeOtherColMatching = cmpThis.IsEqualTo(&corr->comparanda[iCol]))
								break;
						}
						if (!isSomeOtherColMatching) continue;
					}

					bool isMeaningOK = true;
					if (doLookMeaning)
					{
						for (int iCol = 0; iCol < nDicts; iCol++)
						{
							if (isMeaningOK = (corr->comparanda[iCol].wf && !wordThis->CompareTranslationWith(corr->comparanda[iCol].wf, 3)))
								break;
						}
					}

					if (isMeaningOK)
					{
						if (!isMatchingRows)
						{
							OutputLanguageHeader(trOut);
							trOut->Add(L"Уже имеющиеся ряды", IT_COLUMN);
							trOut->HorLine();

							isMatchingRows = true;
						}

						OutputCorrespondence(corr, trOut);
					}
					if (corr->IsInGroup()) continue;

					nMatchingOrphanRows = LookForMatchingOrphans(wordThis, cndMatch, &cmpThis, corr, wfsOrphans, nsOrphans, doLookMeaning, nMatchingOrphanRows, maxnWF);
				}
			}
			if (nMatchingOrphanRows)
			{
				trOut->HorLine();
				trOut->Add(L"Слова-сироты", IT_COLUMN);
				if (!isMatchingRows)
					OutputLanguageHeader(trOut);
				else
					trOut->HorLine();
				for (int iRow = 0; iRow < nMatchingOrphanRows; iRow++)
				{
					for (int i = 0; i < nDicts; i++)
					{
						if (iRow < nsOrphans[i])
						{
							WordForm**wfs = wfsOrphans[i];
							/////////////////////////////СДЕЛАТЬ!
							trOut->Add(wfs[iRow]->formOrig, IT_TAB);
							trOut->Add(wfs[iRow]->wordTranslation, IT_MARRQUOTES | IT_TAB);
						}
						else
						{
							trOut->Add(NULL, IT_TAB);
							trOut->Add(NULL, IT_TAB);
						}
					}
				}
			}

			if (isMatchingRows || nMatchingOrphanRows)
				trOut->HorLine();
			else
				trOut->Add(L"НЕТ", IT_LINEBRKAFTER);
			//if (isMatchingRows) break;									
		}
		//trOut->HorLine();
	//}

	//trOut->HorLine();
		for (int i = 0; i < nDicts; i++) delete[] wfsOrphans[i];
		delete[] wfsOrphans;
		delete[] nsOrphans;
	}


	int LookForMatchingOrphans(WordForm* wordThis, Condition* cndMatch, Comparandum* cmpMatching, Correspondence* corr, WordForm*** wfsOrphans, int* nsOrphans, bool doLookMeaning, int nMatchingOrphanRows, int maxnWF)
	{
		;

		for (int iDictOrphan = 0; iDictOrphan < nDicts; iDictOrphan++)
		{
			Dictionary* dic = Dict(iDictOrphan);

			if (corr)
				cmpMatching = &corr->comparanda[iDictOrphan];

			WordForm* wordOrphan;
			for (BTree::Walker w(&dic->trWordForms); wordOrphan = (WordForm*)w.Next();)
			{
				if (wordOrphan == wordThis) continue;
				if (wordOrphan->flags & WF_HASLINK) continue;

				Comparandum cmpOrphan(wordOrphan);
				cmpOrphan.typeOfSegment = cndMatch->GetFirstMatchingFragment(
					dic->ipa,
					&cmpOrphan.sound,
					(cmpOrphan.wf ? cmpOrphan.wf->formIPA : NULL),
					cmpOrphan.chrFragment);

				if (!cmpOrphan.IsEqualTo(cmpMatching)) continue;
				if (doLookMeaning && wordThis->CompareTranslationWith(wordOrphan, 3)) continue;
				if (nsOrphans[iDictOrphan] >= maxnWF) continue;

				//СДЕЛАТЬ!!!
				//wfsOrphans[iDictOrphan][nsOrphans[iDictOrphan]++] = wordOrphan;

				WordForm**wfs = wfsOrphans[iDictOrphan];

				bool isFound = false;
				for (int i = 0; i < nsOrphans[iDictOrphan]; i++)
				{
					if (wfs[i] == wordOrphan)
					{
						isFound = true;
						break;
					}
				}

				if (!isFound)
				{
					int ii = nsOrphans[iDictOrphan];
					nsOrphans[iDictOrphan]++;
					wfs[ii] = wordOrphan;
					if (nMatchingOrphanRows < nsOrphans[iDictOrphan])
						nMatchingOrphanRows = nsOrphans[iDictOrphan];
				}
			}
		}
		return nMatchingOrphanRows;
	}

	void Output(InfoTree* trOut)
	{
		LPTSTR word;
		Sound* sound;

		InfoNode* inTo;

		inTo = trOut->Add(L"Предложения" /*— только по переводу"*/, IT_COLUMN | IT_LINEBRKBEFORE, NULL, false);
		trOut->Add(NULL, IT_HORLINE, inTo);

		Correspondence* c;
		//for (CorrespondenceTree::Iterator it(&tCorrespondences); c = it.Next();)
		//{
		for (int iRow = 0; iRow < nRowsCorresp; iRow++)
		{
			c = &corresps[iRow];
			//if (it.IsStartOfGroup())
			//{
			//	OutputSoundsHeader(c, trOut, inTo, true, false, IT_TAB, IT_HORLINE);
			//}


			for (int iCol = 0; iCol < nDicts; iCol++)
			{
				WordForm* wf = c->comparanda[iCol].wf;
				trOut->Add((wf ? wf->formOrig : NULL), IT_TAB, inTo);
				//trOut->Add(c->comparanda[iCol].formIPA, IT_TAB, inTo);
				trOut->Add((wf ? wf->wordTranslation : NULL), IT_MARRQUOTES | IT_TAB, inTo);
			}

			//if (it.IsEndOfGroup())
			//{
			//	trOut->Add(NULL, IT_HORLINE, inTo);
			//}
		}

		trOut->Add(NULL, IT_HORLINE, inTo);
		trOut->Add(NULL, IT_SECTIONBRK, inTo);
	}
};