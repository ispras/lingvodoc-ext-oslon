class ComparisonWithGuess : public Comparison
{
	int 	nRowsCorresp;
	int		nRowsNoCorresp;
public:
	ComparisonWithGuess(int _nRowsCorresp, int _nRowsNoCorresp, int nCols) : Comparison((_nRowsCorresp + _nRowsNoCorresp) * 2, nCols)
	{
		nRowsCorresp = _nRowsCorresp;
		nRowsNoCorresp = _nRowsNoCorresp;
	}
	void Input(LPTSTR sIn, bool hasPhonData = false, int begCols = 0, int nCols = 0, int nColsAll = 0)
	{
		Parser parser(sIn, L"\0", PARSER_NONNULLEND);
		LPTSTR wordOrig = NULL, wordIPA = NULL, wordTranslation = NULL, wchrTranscr = NULL, wLength = NULL, wF1 = NULL, wF2 = NULL, wF3 = NULL;
		WordForm* wfFirstInRow;

		if (!nColsAll) nColsAll = nDicts;
		if (!nCols) nCols = nDicts;

		int iColIn = -1, iCol = -1, iRow = -1;
		while (parser.Next())
		{
			if (!NextCol(iColIn, iRow, nColsAll, nRowsCorresp + nRowsNoCorresp)) break;
			if (iColIn >= begCols && iColIn < begCols + nCols)
			{
				iCol = iColIn - begCols;

				Dictionary* dic = Dict(iCol);
				if (iCol == 0)
					wfFirstInRow = NULL;

				if (iRow == -1)
					dic->GetDictInfo(parser);
				else
				{
					dic->GetOrigIPAAndTranslation(parser, wordOrig, wordIPA, wordTranslation, hasPhonData, wchrTranscr, wLength, wF1, wF2, wF3);
					//new (&corresps[iRow].comparanda[iCol]) Comparandum(wordIPA, wordOrig, wordTranslation, false, wchrTranscr, wLength, wF1, wF2, wF3);

					if (wordIPA)
					{
						int flag = 0;
						if (iRow < nRowsCorresp)
						{
							if (wfFirstInRow)
							{
								flag = WF_HASLINK;
								wfFirstInRow->flags |= flag;//будет ставиться много раз для первого слова
							}
						}

						WordForm* wfNew = new (dic->pWordForms.New()) WordForm(wordIPA, wordOrig, wordTranslation, flag);
						WordForm* wfFound = (WordForm*)dic->trWordForms.Add(wfNew);
						if (!wfFound) dic->dictinfo.nWords++;

						if (!wfFirstInRow)
							wfFirstInRow = wfNew;
					}
				}
			}
			else//перепрых перевод
			{
				parser.Next();
			}
		}

		EndSubmitForms();
	}
	void Process()
	{
		int iRowCorresp = -1;
		for (int iCol = 0; iCol < nDicts; iCol++)
		{
			WordForm* word;
			Dictionary* dic = Dict(iCol);

			for (BTree::Walker w(&dic->trWordForms); word = (WordForm*)w.Next();)
			{
				if (word->flags & WF_HASLINK) continue;

				iRowCorresp++;

				new (&corresps[iRowCorresp]) Correspondence(nDicts, iRowCorresp);
				new (&corresps[iRowCorresp].comparanda[iCol]) Comparandum(word->formIPA, word->formOrig, word->wordTranslation);

				if (!FindCorrespondences(iCol, word, &corresps[iRowCorresp]))
				{
					free(corresps[iRowCorresp].comparanda);
					iRowCorresp--;
					//corresps[iRow].~Correspondence();
				}
				else if (iRowCorresp == nRowsAll - 1)
					goto Done;
			}
		}
	Done:
		nRowsAll = iRowCorresp + 1;
	}
	int FindCorrespondences(int iColPivot, WordForm* wPivot, Correspondence* c)
	{
		int nFound = 0;
		for (int iCol = 0; iCol < nDicts; iCol++)
		{
			if (iCol == iColPivot) continue;

			WordForm* word;
			Dictionary* dic = Dict(iCol);

			for (BTree::Walker w(&dic->trWordForms); word = (WordForm*)w.Next();)
			{
				if (!wPivot->CompareTranslationWith(word, 3))
				{
					nFound++;
					word->flags |= WF_HASLINK;
					new (&c->comparanda[iCol]) Comparandum(word->formIPA, word->formOrig, word->wordTranslation);
				}
			}
		}
		return nFound;
	}
	void Output(InfoTree* trOut)
	{
		LPTSTR word;
		Sound* sound;

		InfoNode* inTo;

		inTo = trOut->Add(L"Предложения — только по переводу", IT_COLUMN | IT_LINEBRKBEFORE, NULL, false);
		trOut->Add(NULL, IT_HORLINE, inTo);

		Correspondence* c;
		//for (CorrespondenceTree::Iterator it(&tCorrespondences); c = it.Next();)
		//{
		for (int iRow = 0; iRow < nRowsAll; iRow++)
		{
			c = &corresps[iRow];
			//if (it.IsStartOfGroup())
			//{
			//	OutputSoundsHeader(c, trOut, inTo, true, false, IT_TAB, IT_HORLINE);
			//}


			for (int iCol = 0; iCol < nDicts; iCol++)
			{
				trOut->Add(c->comparanda[iCol].formOrig, IT_TAB, inTo);
				//trOut->Add(c->comparanda[iCol].formIPA, IT_TAB, inTo);
				trOut->Add(c->comparanda[iCol].translation, IT_MARRQUOTES | IT_TAB, inTo);
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