class ComparisonWithGuess : public Comparison
{
	//	int 	nRowsCorresp;
	//	int		nRowsNoCorresp;
public:
	ComparisonWithGuess(int _nRowsCorresp, int nCols, int _nRowsNoCorresp) : Comparison(_nRowsCorresp, nCols, _nRowsNoCorresp)
	{
		//	nRowsAll = 0;
	}
	void ProcessAndOutput(InfoTree* trOut, Condition** cndMatch, int nCnd, int iDictThis, bool doLookMeaning)
	{
		TCHAR buf[1000];

		int const maxnWF = 100;

		WordForm*** wfsOrphans = new WordForm * *[nDicts];
		int* nsOrphans = new int[nDicts];
		for (int i = 0; i < nDicts; i++) wfsOrphans[i] = new WordForm * [maxnWF];

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

			for (int i = 0; i < nDicts; i++)
				nsOrphans[i] = 0;
			bool isMatchingRows = false;
			int nMatchingOrphanRows = 0;
			bool isMeaningOK = true;





			if (nRowsNotOrphan < 10)//ПРОИЗВОЛЬНО!
				nMatchingOrphanRows = LookForMatchingOrphans(wordThis, dic, cndMatch, nCnd, false, NULL, wfsOrphans, nsOrphans, true, nMatchingOrphanRows, maxnWF);
			else
			{
				Correspondence* corr;
				for (int iRow = 0; iRow < nRowsCorresp; iRow++)
				{
					corr = &corresps[iRow];
					Comparandum* cmp = &corr->comparanda[iDictThis];

					if (CountEmptyColsInRow(corr) == nDicts - 1)//значит сирота, в т.ч. wordThis
						continue;

					switch (MatchSegment(wordThis, cmp->wf, cndMatch, nCnd, true, dic, dic))
					{
					case ST_EQUAL:
					case ST_BOTHNOTFOUND:
						break;
					case ST_UNEQUAL:
						goto NextRow;
					case ST_ONEEMPTY:
						bool isSomeOtherColMatching = false;
						for (int iCol = 0; iCol < nDicts; iCol++)
						{
							switch (MatchSegment(wordThis, corr->comparanda[iCol].wf, cndMatch, nCnd, true, dic, Dict(iCol)))
							{
							case ST_EQUAL:
								isSomeOtherColMatching = true;
								break;
							}

						}
						if (!isSomeOtherColMatching)
							goto NextRow;

					}

					isMeaningOK = true;
					if (doLookMeaning)
					{
						for (int iCol = 0; iCol < nDicts; iCol++)
						{
							if (isMeaningOK = (corr->comparanda[iCol].wf && !wordThis->CompareTranslationWith(corr->comparanda[iCol].wf, 3)))
							{
								break;
							}
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

					nMatchingOrphanRows = LookForMatchingOrphans(wordThis, NULL, cndMatch, nCnd, true, corr, wfsOrphans, nsOrphans, doLookMeaning, nMatchingOrphanRows, maxnWF);

				NextRow:;
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
							WordForm** wfs = wfsOrphans[i];
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

		}

		//trOut->HorLine();
		for (int i = 0; i < nDicts; i++) delete[] wfsOrphans[i];
		delete[] wfsOrphans;
		delete[] nsOrphans;
	}
	int MatchSegment(WordForm* wordThis, WordForm* wordOther, Condition** cndMatch, int nCnd, bool isAnd, Dictionary* dic1, Dictionary* dic2)
	{
		//bool isEqualAll = isAnd;
		bool wasNotFound = false;
		bool isEqual;
		for (int iCnd = 0; iCnd < nCnd; iCnd++)
		{
			Comparandum cmpThis(wordThis);
			cmpThis.typeOfSegment = cndMatch[iCnd]->GetFirstMatchingFragment(
				dic1->ipa,
				&cmpThis.sound,
				(wordThis ? wordThis->formIPA : NULL),
				cmpThis.chrFragment);


			Comparandum cmpInRow(wordOther);
			cmpInRow.typeOfSegment = cndMatch[iCnd]->GetFirstMatchingFragment(
				dic2->ipa,
				&cmpInRow.sound,
				(wordOther ? wordOther->formIPA : NULL),
				cmpInRow.chrFragment);

			switch (cmpInRow.typeOfSegment)
			{
			case ST_SOUND:
			case ST_FRAGMENT:
				if (cmpThis.typeOfSegment == ST_ERROR)
					return ST_UNEQUAL;

				isEqual = cmpThis.IsEqualTo(&cmpInRow, true, true);
				break;
			case ST_ERROR:
				wasNotFound = isEqual = (cmpThis.typeOfSegment == ST_ERROR && iCnd == 0);//ни там, ни там не нашли первого согласного (но это временно, ибо надо прописывать в условии)
				if (isEqual)
					continue;
				else
					return ST_UNEQUAL;
				break;
			case ST_EMPTYAUTOFILL://такого не бывает?
			case ST_NONE:
				return ST_ONEEMPTY;
			default:
				return ST_ERROR;
			}



			if (isAnd)
			{
				if (!isEqual)
					return ST_UNEQUAL;
			}
			else /*!isAnd*/
			{
				if (isEqual)
					return ST_EQUAL;
				else if (wasNotFound)
					return ST_UNEQUAL;
			}
		}
		if (isAnd)
			return ST_EQUAL;
		else
			return ST_UNEQUAL;
	}

	int LookForMatchingOrphans(WordForm* wThis, Dictionary* dicThis, Condition** cndMatch, int nCnd, bool isAnd, Correspondence* corr, WordForm*** wfsOrphans, int* nsOrphans, bool doLookMeaning, int nMatchingOrphanRows, int maxnWF)
	{
		for (int iDictOrphan = 0; iDictOrphan < nDicts; iDictOrphan++)
		{
			Dictionary* dic = Dict(iDictOrphan);
			Dictionary* dicMatching = corr ? Dict(iDictOrphan) : dicThis;
			WordForm* wMatching = corr ? corr->comparanda[iDictOrphan].wf : wThis;
			if (!wMatching) wMatching = wThis;

			WordForm* wOrphan;
			for (BTree::Walker w(&dic->trWordForms); wOrphan = (WordForm*)w.Next();)
			{
				if (wOrphan == wThis) continue;
				if (wOrphan->flags & WF_HASLINK) continue;


				switch (MatchSegment(wMatching, wOrphan, cndMatch, nCnd, isAnd, dicMatching, dic))
				{
				case ST_EQUAL:
					break;
				default:
					continue;
				}


				if (doLookMeaning && wThis->CompareTranslationWith(wOrphan, 3)) continue;
				if (nsOrphans[iDictOrphan] >= maxnWF) continue;

				//СДЕЛАТЬ!!!
				//wfsOrphans[iDictOrphan][nsOrphans[iDictOrphan]++] = wordOrphan;

				WordForm** wfs = wfsOrphans[iDictOrphan];

				bool isFound = false;
				for (int i = 0; i < nsOrphans[iDictOrphan]; i++)
				{
					if (wfs[i] == wOrphan)
					{
						isFound = true;
						break;
					}
				}

				if (!isFound)
				{
					int ii = nsOrphans[iDictOrphan];
					nsOrphans[iDictOrphan]++;
					wfs[ii] = wOrphan;
					if (nMatchingOrphanRows < nsOrphans[iDictOrphan])
						nMatchingOrphanRows = nsOrphans[iDictOrphan];
				}
			}
		}
		return nMatchingOrphanRows;
	}

	void Output___(InfoTree* trOut)
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