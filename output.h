class CognateList
{
public:
	float l_, f1_, f2_, f3_;
	TCHAR L[30];
	TCHAR F1[30];
	TCHAR F2[30];
	TCHAR F3[30];
	int n;
	InfoTree* trOut;
	InfoNode *inMult;
	InfoTree* trCld;

	CognateList(InfoTree* _trOut, InfoNode *_inMult, InfoTree* _trCld = NULL)
	{
		trOut = _trOut;
		inMult = _inMult;
		trCld = _trCld;
		l_ = f1_ = f2_ = f3_ = 0;
		n = 0;
	}
	void Add(LPTSTR l, LPTSTR f1, LPTSTR f2, LPTSTR f3)
	{
		if (!l || !f1 || !f2 || !f3)
			return;

		_addcalc(&l_, l);
		_addcalc(&f1_, f1);
		_addcalc(&f2_, f2);
		_addcalc(&f3_, f3);

		if (trCld)
		{
			trCld->Add(l, IT_TAB);
			trCld->Add(f1, IT_TAB);
			trCld->Add(f2, IT_TAB);
			trCld->Add(f3, IT_TAB);
		}

		n++;
	}
	void Done()
	{
		_donecalc(&l_, L, n);
		_donecalc(&f1_, F1, n);
		_donecalc(&f2_, F2, n);
		_donecalc(&f3_, F3, n);
	}
	void Output()
	{
		Done();
		trOut->Add(L"Средние", IT_COLUMN | IT_TAB, inMult);
		trOut->Add(NULL, IT_TAB, inMult);
		trOut->Add(NULL, IT_TAB, inMult);
		trOut->Add(L, IT_TAB, inMult);
		trOut->Add(F1, IT_TAB, inMult);
		trOut->Add(F2, IT_TAB, inMult);
		trOut->Add(F3, IT_TAB, inMult);
	}
};
//////////////////////////////////

void Comparison::OutputLanguageList(InfoTree* trOut)
{
	TCHAR buf[500];
	int f = 0;//IT_EMPTYLINEBEFORE; 
	trOut->Add(NULL, IT_HORLINE);
	for (int i = 0; i < nDicts; i++)
	{
		_ltow(i + 1, buf, 10);
		wcscat(buf, L": ");
		if (Dict(i)->dictinfo.name)
			wcscat(buf, Dict(i)->dictinfo.name);

		if (nRowsAll)
		{
			wcscat(buf, L" (");
			strcati(buf, Dict(i)->dictinfo.nWords);
			wcscat(buf, L" форм = ");
			strcati(buf, (Dict(i)->dictinfo.nWords * 100) / nRowsAll);
			wcscat(buf, L"% от числа соотв.)");
		}

		trOut->Add(buf, IT_LINEBRKAFTER | f);
		//trOut->Add(IT_LINEBRK, ndTo);
		f = 0;
	}
	trOut->Add(NULL, IT_HORLINE);
}
void Comparison::SoundCorrespondenceNumbers(InfoTree* trOut, int threshold)
{
	TCHAR buf[500];

	wcscpy(buf, L"Барьер: ");
	strcati(buf, threshold);
	wcscat(buf, L"%");

	trOut->Add(buf, IT_LINEBRKAFTER);

	for (int i = 0; i < nDicts; i++)
	{
		_ltow(i + 1, buf, 10);
		wcscat(buf, L": ");

		if (nSoundCorresp)
		{
			strcati(buf, Dict(i)->dictinfo.nFilledSoundCorresp);
			wcscat(buf, L" звуков в неед. рядах = ");
			strcati(buf, (Dict(i)->dictinfo.nFilledSoundCorresp * 100) / nSoundCorresp);
			wcscat(buf, L"% от числа неед. рядов)");
		}

		trOut->Add(buf, IT_LINEBRKAFTER);
	}
}
void Comparison::OutputLanguageHeader(InfoTree* trOut, InfoNode* ndTo, bool isProtoSounds)
{
	int iCol;
	//	if (isProtoSounds)
	//		iCol = -1;
	//	else
	iCol = 0;

	LPTSTR word;
	trOut->Add(NULL, IT_HORLINE, ndTo);
	for (; iCol < nDicts; iCol++)
	{
		//		if (iCol == -1)
		//			trOut->Add(L"праформа", IT_TAB, ndTo);
		//		else
		//		{
		TCHAR buf[500];
		_ltow(iCol + 1, buf, 10);
		wcscat(buf, L": ");
		wcscat(buf, Dict(iCol)->dictinfo.name);
		trOut->Add(buf, IT_TAB, ndTo);
		trOut->Add(L"", IT_TAB, ndTo);
		//		}
	}
	trOut->Add(NULL, IT_HORLINE, ndTo);
}
void Comparison::OutputPhoneticHeader(InfoTree* trOut, InfoNode* ndTo)
{
	trOut->Add(NULL, IT_HORLINE, ndTo);
	trOut->Add(L"словоформа", IT_TAB, ndTo);
	trOut->Add(L"перевод", IT_TAB, ndTo);
	trOut->Add(L"гласный", IT_TAB, ndTo);
	trOut->Add(L"t", IT_TAB, ndTo);
	trOut->Add(L"f₁", IT_TAB, ndTo);
	trOut->Add(L"f₂", IT_TAB, ndTo);
	trOut->Add(L"f₃", IT_TAB, ndTo);
	trOut->Add(NULL, IT_HORLINE, ndTo);
}
void Comparison::OutputSoundsHeader(Correspondence* c, InfoTree* trOut, InfoNode* inTo, bool skipTransl, bool onlyWithForms, int fSep, int fLine)
{
	int iCol = 0;

	for (; iCol < nDicts; iCol++)
	{
		LPTSTR word;
		int fAdd = 0;
		TCHAR buf[10];
		/*
				if (iCol == 0 && isProtoSounds)
				{
					fAdd |= IT_ASTERISK|IT_GREATER|(fSep & IT_TAB);

					if (c->comparanda[iCol].sound)
						word = c->comparanda[iCol].Text();
					else
						word = NULL;
				}
				else
				{
		*/
		bool isProtoCol = (iCol == 0 && c->comparanda[iCol].isReconstructed);
		bool isLastCol = (iCol == nDicts - 1);

		fAdd |= fSep * (!isLastCol) * (!isProtoCol);
		fAdd |= IT_GREATER * isProtoCol;

		bool isSoundOK;
		if (onlyWithForms)
			isSoundOK = !!c->comparanda[iCol].wf;
		else
			isSoundOK = c->comparanda[iCol].isSoundInCognates;

		if (isSoundOK)
		{
			word = c->comparanda[iCol].Text();

			fAdd |= IT_ASTERISK * c->comparanda[iCol].isReconstructed;

			if (word)
				fAdd |= IT_SQRBRK * !c->comparanda[iCol].isReconstructed;
		}
		else word = L" ? ";
		//}

		if (!word) word = L" 0 ";//Ø";

		trOut->Add(word, fAdd, inTo);

		if (skipTransl)
			trOut->Add(L"", IT_TAB, inTo);
	}
	trOut->Add(NULL, fLine, inTo);
}
void Comparison::OutputCognatesRow(Correspondence* c, InfoTree* trOut, InfoNode* inTo, bool isPhonData, int fLine)
{
	for (int iCol = 0; iCol < nDicts; iCol++)
	{
		OutputCognate(&c->comparanda[iCol], trOut, inTo, isPhonData, fLine, NULL);
	}
	if (fLine)
		trOut->Add(NULL, fLine, inTo);
}

void Comparison::OutputCognate(Comparandum* cmp, InfoTree* trOut, InfoNode* inTo, bool isPhonData, int fLine, CognateList* cl)
{
	trOut->Add((cmp->wf ? cmp->wf->formOrig : NULL), IT_TAB, inTo);
	trOut->Add((cmp->wf ? cmp->wf->wordTranslation : NULL), IT_MARRQUOTES | IT_TAB, inTo);

	if (isPhonData)
	{
		trOut->Add(cmp->chrTranscr, IT_TAB, inTo);
		trOut->Add(cmp->wLength, IT_TAB, inTo);
		trOut->Add(cmp->wF1, IT_TAB, inTo);
		trOut->Add(cmp->wF2, IT_TAB, inTo);
		trOut->Add(cmp->wF3, IT_TAB, inTo);

		if (cl)
		{
			cl->Add(cmp->wLength, cmp->wF1, cmp->wF2, cmp->wF3);
		}
	}

	if (fLine)
		trOut->Add(NULL, fLine, inTo);
}

void Comparison::OutputCognatesBySound(Correspondence* cGroupTop, Correspondence* cOther, int iColDiff, InfoTree* trOut, InfoNode* inMult, InfoTree* trCld, Correspondence* cEqual)
{
	if (trCld)
	{

		//if (!cEqual->comparanda[iColDiff].sound)
		//{
		//trCld->Add(L"?", IT_SPACE);
		/////////////////////////////////////////////////
		//out(cEqual->comparanda[iColDiff].formIPA);
		/////////////////////////////////////////////////
		//}
		//else
		trCld->Add(cEqual->comparanda[iColDiff].Text(), IT_SQRBRK | IT_SPACE);
		trCld->Add(L"в других рядах", IT_LINEBRKAFTER);
	}


	bool isRegExtra = false;
	bool isRows = false;

	Correspondence* cExtra;
	for (CorrespondenceTree::Iterator itExtra(&tCorrespondences, true); cExtra = itExtra.Next();) //всё верно
	{
		if (cExtra == cGroupTop)
			continue;
		if (cExtra == cOther)
			continue;

		if (cExtra->comparanda[iColDiff].IsEqualTo(&cEqual->comparanda[iColDiff]))
		{
			isRegExtra = false;

			if (itExtra.IsStartOfGroup())
			{
				CognateList cl(trOut, inMult, trCld);
				for (itExtra.TryEnterGroup(); cExtra; cExtra = itExtra.Next())
				{
					if (cExtra->comparanda[iColDiff].wf)
					{
						if (!isRegExtra)
						{
							isRegExtra = true;

							trOut->Add(cEqual->comparanda[iColDiff].Text(), IT_SQRBRK | IT_SPACE, inMult);
							trOut->Add(L"также в ряду", IT_COLUMN | IT_TAB, inMult);
							OutputSoundsHeader(cExtra, trOut, inMult, false, false, IT_DASH, IT_HORLINE);
						}

						OutputCognate(&cExtra->comparanda[iColDiff], trOut, inMult, true, IT_LINEBRK, &cl);
					}
					if (itExtra.IsEndOfGroup())
						break;
				}
				if (isRegExtra)
				{
					trOut->Add(NULL, IT_HORLINE, inMult);
					if (cl.n > 1)
					{
						cl.Output();
						trOut->Add(NULL, IT_HORLINE, inMult);
					}
					if (cl.n > 0)
						isRows = true;
				}
			}
		}

		//			if (it.IsEndOfGroup())
		//			{
		//				trOut->Add(NULL, IT_HORLINE, inMult);
		//				inTo = inOnce;
		//			}
	}

	//		trOut->Add(NULL, IT_HORLINE, inMult);

	if (trCld && !isRows)
		trCld->Add(NULL, IT_EMPTYLINEBEFORE);

}

void Comparison::OutputDeviationsWithMaterial(Condition* cnd, InfoTree* trOut, InfoTree* trCld)
{
	InfoNode* inCnd, *inMult;//, *inTo;//, *inOnce, *inMultList;

	inMult = inCnd = trOut->Add(cnd->title, IT_COLUMN | IT_LINEBRKBEFORE | IT_LINEBRKAFTER, NULL, false, cnd);

	OutputPhoneticHeader(trOut, inMult);

	int nDeviationGroups = 0;
	int nDeviations;
	TCHAR bufn[300];

	Correspondence* cGroup, *cGroupTop;
	for (CorrespondenceTree::Iterator itGroup(&tCorrespondences, true); cGroupTop = cGroup = itGroup.Next();) //всё верно, надо сразу на следующий прыгать (он сразу будет первым)
	{
		if (cGroup->dataExtra)//уже был в отклонениях (т.е. в ряду №2)
			continue;
		if (!itGroup.IsStartOfGroup())
			continue;

		bool isDeviations = false;

		Correspondence* cOther;
		for (CorrespondenceTree::Iterator itOther(&tCorrespondences, true); cOther = itOther.Next();) //всё верно
		{

			int nDiff = 0, iColDiff;
			for (int iCol = 0; (iCol < nDicts) && nDiff <= 1; iCol++)
			{
				if (cOther->comparanda[iCol].wf)
				{
					if (!cGroupTop->comparanda[iCol].IsEqualTo(&cOther->comparanda[iCol]))
					{
						nDiff++;
						iColDiff = iCol;
					}
				}
			}

			if (nDiff == 1)
			{
				if (cGroupTop->comparanda[iColDiff].sound)
				{
					bool isSomethingInGroup = false;
					for (itGroup.TryEnterGroup(); cGroup; cGroup = itGroup.Next())
					{
						if (cGroup->comparanda[iColDiff].wf)
						{
							isSomethingInGroup = true;
							break;
						}
						if (itGroup.IsEndOfGroup())
							break;
					}
					itGroup.TryExitGroup();


					if (!isSomethingInGroup)
						continue;


					if (!isDeviations)
					{
						isDeviations = true;

						nDeviationGroups++;

						nDeviations = L'A';
					}
					_ltow(nDeviationGroups, bufn, 10);

					lstrcat(bufn, L".");
					strcatb(bufn, nDeviations);

					lstrcat(bufn, L" ►►►");
					trOut->Add(bufn, IT_EMPTYLINEBEFORE | IT_LINEBRKAFTER, inMult);
					trOut->Add(NULL, IT_HORLINE, inMult);

					trOut->Add(L"Словарь", IT_COLUMN | IT_SPACE, inMult);
					trOut->Add(Dict(iColDiff)->dictinfo.name, IT_LINEBRKAFTER, inMult);
					trOut->Add(NULL, IT_HORLINE, inMult);

					trOut->Add(L"Простейший ряд", IT_COLUMN | IT_TAB, inMult);
					OutputSoundsHeader(cGroupTop, trOut, inMult, false, false, IT_DASH, IT_LINEBRKAFTER);

					trOut->Add(NULL, IT_HORLINE, inMult);



					lstrcpy(bufn, L"График ");
					strcati(bufn, nDeviationGroups);
					lstrcat(bufn, L".");
					strcatb(bufn, nDeviations);
					trCld->Add(bufn, IT_COLUMN | IT_SPACE);//лучше разными вызовами Add и убрать к чёрту bufn

					trCld->Add(cGroupTop->comparanda[iColDiff].Text(), IT_SQRBRK | IT_SPACE);
					trCld->Add(L":", IT_SPACE);
					trCld->Add(cOther->comparanda[iColDiff].Text(), IT_SQRBRK | IT_LINEBRKAFTER);




					trCld->Add(cGroupTop->comparanda[iColDiff].Text(), IT_SQRBRK | IT_SPACE);
					trCld->Add(L"в простейшем ряду", IT_COLUMN | IT_SPACE);
					OutputSoundsHeader(cGroupTop, trCld, NULL, false, true, IT_DASH, IT_LINEBRKAFTER);

					nDeviations++;


					//единичные не берём

					{
						CognateList cl(trOut, inMult, trCld);

						for (itGroup.TryEnterGroup(); cGroup; cGroup = itGroup.Next())
						{
							if (cGroup->comparanda[iColDiff].wf)
							{
								OutputCognate(&cGroup->comparanda[iColDiff], trOut, inMult, true, IT_LINEBRK, &cl);
							}
							if (itGroup.IsEndOfGroup())
								break;
						}
						itGroup.TryExitGroup();

						trOut->Add(NULL, IT_HORLINE, inMult);

						if (cl.n > 1)
						{
							cl.Output();
							trOut->Add(NULL, IT_HORLINE, inMult);
						}
						else if (cl.n == 0)
							trCld->Add(NULL, IT_EMPTYLINEBEFORE);
					}





					trOut->Add(L"Отклонение", IT_COLUMN | IT_SPACE, inMult);
					trOut->Add(cGroupTop->comparanda[iColDiff].Text(), IT_SQRBRK, inMult);
					trOut->Add(L" : ", 0, inMult);
					trOut->Add(cOther->comparanda[iColDiff].Text(), IT_SQRBRK | IT_LINEBRKAFTER, inMult);

					trOut->Add(NULL, IT_LINEBRK, inMult);

					trOut->Add(L"Отклоняющийся ряд", IT_COLUMN | IT_TAB, inMult);
					cOther->dataExtra = (void*)1;
					OutputSoundsHeader(cOther, trOut, inMult, false, true, IT_DASH, IT_HORLINE);



					trCld->Add(cOther->comparanda[iColDiff].Text(), IT_SQRBRK | IT_SPACE);
					trCld->Add(L"в отклоняющемся ряду", IT_COLUMN | IT_SPACE);
					OutputSoundsHeader(cOther, trCld, NULL, false, true, IT_DASH, IT_LINEBRKAFTER);



					{
						bool isRows = false;
						CognateList cl(trOut, inMult, trCld);
						for (itOther.TryEnterGroup(); cOther; cOther = itOther.Next())
						{
							if (cOther->comparanda[iColDiff].wf)
							{
								OutputCognate(&cOther->comparanda[iColDiff], trOut, inMult, true, IT_LINEBRK, &cl);
							}
							if (!itOther.AreWeInsideGroup() || itOther.IsEndOfGroup())
								break;
						}


						trOut->Add(NULL, IT_HORLINE, inMult);
						if (cl.n > 1)
						{
							cl.Output();
							trOut->Add(NULL, IT_HORLINE, inMult);
						}
						if (cl.n == 0) trCld->Add(NULL, IT_EMPTYLINEBEFORE);

						OutputCognatesBySound(cGroupTop, cOther, iColDiff, trOut, inMult, trCld, cGroupTop);
						OutputCognatesBySound(cGroupTop, cOther, iColDiff, trOut, inMult, trCld, cOther);
					}

					trCld->Add(NULL, IT_SECTIONBRK);
				}
			}
		}
		if (isDeviations)
			trOut->Add(NULL, IT_HORLINE, inMult);

	}
	//		trOut->Add(NULL, IT_HORLINE, inMult);
	trOut->Add(NULL, IT_SECTIONBRK, inMult);
}
void Comparison::OutputCorrespondence(Correspondence* c, InfoTree* trOut, InfoNode* inTo)
{
	for (int iCol = 0; iCol < nDicts; iCol++)
	{
		WordForm* wf = c->comparanda[iCol].wf;

		trOut->Add((wf ? wf->formOrig : NULL), IT_TAB | (c->comparanda[iCol].isSingleInGroup*IT_PARENTHESES), inTo);
		trOut->Add((wf ? wf->wordTranslation : NULL), IT_MARRQUOTES | IT_TAB, inTo);
	}
}

void Comparison::OutputCorrespondencesWithMaterial(Condition* cnd, InfoTree* trOut, bool doMakeTablesForSingles)
{
	LPTSTR word;
	Sound* sound;

	InfoNode* inCnd, *inMultGood, *inMultBad, *inOnce, *inMultListGood, *inMultListBad;

	inCnd = trOut->Add(cnd->title, IT_COLUMN | IT_LINEBRKBEFORE, NULL, false, cnd);

	bool wasGoodGroup = false,
		wasBadGroup = false;

	if (!doMakeTablesForSingles)
	{
		inMultListGood = trOut->Add(L"Надёжные ряды", IT_COLUMN | IT_EMPTYLINEBEFORE | IT_LINEBRKAFTER, inCnd);
		inMultListBad = trOut->Add(L"Сомнительные ряды", IT_COLUMN | IT_EMPTYLINEBEFORE | IT_LINEBRKAFTER, inCnd);

		inMultGood = trOut->Add(L"Материал — надёжные ряды", IT_COLUMN | IT_EMPTYLINEBEFORE | IT_LINEBRKAFTER, inCnd);
		OutputLanguageHeader(trOut, inMultGood, false);

		inMultBad = trOut->Add(L"Материал — сомнительные ряды", IT_COLUMN | IT_EMPTYLINEBEFORE | IT_LINEBRKAFTER, inCnd);
		OutputLanguageHeader(trOut, inMultBad, false);

		inOnce = trOut->Add(L"Материал — единичные ряды", IT_COLUMN | IT_EMPTYLINEBEFORE | IT_LINEBRKAFTER, inCnd);
		OutputLanguageHeader(trOut, inOnce, false);
	}
	else
	{
		inMultListBad = inMultListGood = trOut->Add(L"Оглавление", IT_COLUMN | IT_EMPTYLINEBEFORE | IT_LINEBRKAFTER, inCnd);
		inMultBad = inMultGood = trOut->Add(L"Материал", IT_COLUMN | IT_EMPTYLINEBEFORE | IT_LINEBRKAFTER, inCnd);
		inOnce = NULL;// inMult;
		//trOut->Add(NULL, IT_HORLINE, inMult);
	}

	InfoNode* inTo = inOnce;

	Correspondence* c;
	for (CorrespondenceTree::Iterator it(&tCorrespondences); c = it.Next();)
	{
		//НЕ РАЗБ.!
		//if (it.IsStartOfGroup() || (doMakeTablesForSingles && !c->IsInGroup()))

		switch (it.IsStartOfGroup())
		{
		case false:
			if (c->IsInGroup() || !doMakeTablesForSingles) break;
		case true:
			if (c->isDoubtful)
				wasBadGroup = true;
			else
				wasGoodGroup = true;
			OutputSoundsHeader(c, trOut, c->isDoubtful ? inMultListBad : inMultListGood, false, false, IT_DASH, IT_LINEBRK);
			inTo = c->isDoubtful ? inMultBad : inMultGood;
			OutputSoundsHeader(c, trOut, inTo, true, false, IT_TAB, IT_HORLINE);
		}

		OutputCorrespondence(c, trOut, inTo);

		switch (it.IsEndOfGroup())
		{
		case false:
			if (c->IsInGroup() || !doMakeTablesForSingles) break;
		case true:
			trOut->Add(NULL, IT_HORLINE, inTo);
			if (!doMakeTablesForSingles)
				inTo = inOnce;
		}
	}
	if (!doMakeTablesForSingles)
	{
		trOut->Add(NULL, IT_HORLINE, inOnce);
		trOut->Add(NULL, IT_SECTIONBRK, inOnce);
	}

	if (!wasBadGroup)
		trOut->Add(L"НЕТ", 0, inMultListBad);
}

void Comparison::OutputReconstructedWords(InfoTree* trOut)//нельзя тут повторять
{
	InfoNode* inMult = trOut->Add(L"Реконструкции", IT_COLUMN | IT_EMPTYLINEBEFORE | IT_LINEBRKAFTER, NULL);
	OutputLanguageHeader(trOut, inMult, true);

	for (int iRow = 0; iRow < nRowsAll; iRow++)
	{
		//if (formIPACur = cmp[0].corresps[iRow].comparanda[iCol].formIPA)
		for (int iCol = 0; iCol < nDicts; iCol++)
		{
			if (corresps[iRow].comparanda[0].wf)//реконструкция
			{
				WordForm* wf = corresps[iRow].comparanda[iCol].wf;
				trOut->Add((wf ? wf->formOrig : NULL), IT_TAB | (IT_ASTERISK*(corresps[iRow].comparanda[iCol].isReconstructed)), inMult);
				trOut->Add((wf ? wf->wordTranslation : NULL), IT_MARRQUOTES | IT_TAB, inMult);
			}
		}
	}
	trOut->Add(NULL, IT_HORLINE, inMult);
	trOut->Add(NULL, IT_SECTIONBRK, inMult);
}
void Comparison::OutputReconstructedSounds(Condition* cnd, InfoTree* trOut)//нельзя тут повторять
{
	LPTSTR word;
	Sound* sound;

	InfoNode* inCnd, *inMult,/* *inOnce,*/ *inMultList;

	inMultList = trOut->Add(cnd->title, IT_COLUMN | IT_LINEBRKBEFORE | IT_LINEBRKAFTER, NULL, false, cnd);
	inMult = trOut->Add(NULL, IT_COLUMN | IT_HORLINE);

	InfoNode* inTo = inMult;

	Correspondence* c;
	for (CorrespondenceTree::Iterator it(&tCorrespondences); c = it.Next();)
	{
		if (it.IsStartOfGroup())
			//if (it.IsStartOfGroup() || c->rankAllSoundsSame >= 5)
		{
			OutputSoundsHeader(c, trOut, inMultList, false, false, IT_DASH, IT_LINEBRK);
		}

		it.TryExitGroup();
	}
	trOut->Add(NULL, IT_LINEBRKAFTER, inMult);
}

void Comparison::OutputDistances(Condition* cnd, DistanceMatrix* mtx, InfoTree* trOut)
{
	LPTSTR word;
	Sound* sound;

	InfoNode* inCnd, *inMtx;

	inCnd = trOut->Add(cnd->title, IT_COLUMN | IT_LINEBRKBEFORE | IT_LINEBRKAFTER, NULL, false, cnd);
	trOut->Add(NULL, IT_LINEBRK, inCnd);
	trOut->Add(NULL, IT_LINEBRK, inCnd);

	trOut->Add(NULL, IT_TAB, inCnd);
	for (int iCol = 0; iCol < nDicts; iCol++)
		trOut->Add(Dict(iCol)->dictinfo.name, IT_TAB, inCnd);
	inMtx = trOut->Add(NULL, IT_LINEBRK, inCnd);

	for (int iRow = 0; iRow < nDicts; iRow++)
	{
		trOut->Add(Dict(iRow)->dictinfo.name, IT_TAB, inMtx);

		for (int iCol = 0; iCol < nDicts;/*iRow;*/ iCol++)
		{
			TCHAR bufn[20];
			trOut->Add(strcpyi(bufn, mtx->langs[iRow].dist[iCol]), IT_TAB, inMtx);
		}

		trOut->Add(NULL, IT_LINEBRK, inMtx);
	}

	trOut->Add(NULL, IT_HORLINE, inCnd);
	trOut->Add(NULL, IT_SECTIONBRK, inCnd);
}
