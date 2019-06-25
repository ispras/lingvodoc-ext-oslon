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
		if (dictinfos[i].name)
			wcscat(buf, dictinfos[i].name);

		if (nCorresp)
		{
			wcscat(buf, L" (");
			strcati(buf, dictinfos[i].nWords);
			wcscat(buf, L" форм = ");
			strcati(buf, (dictinfos[i].nWords * 100) / nCorresp);
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
			strcati(buf, dictinfos[i].nFilledSoundCorresp);
			wcscat(buf, L" звуков в неед. рядах = ");
			strcati(buf, (dictinfos[i].nFilledSoundCorresp * 100) / nSoundCorresp);
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
		trOut->Add(dictinfos[iCol].name, IT_TAB, ndTo);
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
void Comparison::OutputSoundsHeader(Correspondence* c, InfoTree* trOut, InfoNode* inTo, bool isProtoSounds, bool skipTransl, bool onlyWithForms, int fSep, int fLine)
{
	int iCol;
	//	if (isProtoSounds)
	//		iCol = -1;
	//	else
	iCol = 0;

	for (; iCol < nDicts; iCol++)
	{
		LPTSTR word;

		int fAdd;

		TCHAR buf[10];

		//if (iCol == -1)
		if (iCol == 0 && isProtoSounds)
		{
			fAdd = IT_GREATER | IT_ASTERISK | (fSep & IT_TAB);
			//lstrcpy(buf, L"*");
			//word = buf;
			if (c->comparanda[iCol].sound)
				word = c->comparanda[iCol].Text();
			else
				word = NULL;
		}
		else
		{
			if (iCol < nDicts - 1)
				fAdd = fSep;
			else
				fAdd = 0;
			bool isSoundOK;
			if (onlyWithForms)
				isSoundOK = !!c->comparanda[iCol].formIPA;
			else
				isSoundOK = c->comparanda[iCol].isSoundInCognates;

			if (isSoundOK)
			{
				fAdd |= IT_SQRBRK;

				word = c->comparanda[iCol].Text();
			}
			else word = L" ? ";
		}

		if (!word) word = L"@";

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
	trOut->Add(cmp->formOrig, IT_TAB, inTo);
	trOut->Add(cmp->translation, IT_MARRQUOTES | IT_TAB, inTo);

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

		if (!cEqual->comparanda[iColDiff].sound)
		{
			trCld->Add(L"?", IT_SPACE);
			/////////////////////////////////////////////////
			//out(cEqual->comparanda[iColDiff].formIPA);
			/////////////////////////////////////////////////
		}
		else
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
					if (cExtra->comparanda[iColDiff].formOrig)
					{
						if (!isRegExtra)
						{
							isRegExtra = true;

							trOut->Add(cEqual->comparanda[iColDiff].Text(), IT_SQRBRK | IT_SPACE, inMult);
							trOut->Add(L"также в ряду", IT_COLUMN | IT_TAB, inMult);
							OutputSoundsHeader(cExtra, trOut, inMult, false, false, false, IT_DASH, IT_HORLINE);
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
				if (cOther->comparanda[iCol].formIPA)
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
						if (cGroup->comparanda[iColDiff].formOrig)
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
					trOut->Add(dictinfos[iColDiff].name, IT_LINEBRKAFTER, inMult);
					trOut->Add(NULL, IT_HORLINE, inMult);

					trOut->Add(L"Простейший ряд", IT_COLUMN | IT_TAB, inMult);
					OutputSoundsHeader(cGroupTop, trOut, inMult, false, false, false, IT_DASH, IT_LINEBRKAFTER);

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
					OutputSoundsHeader(cGroupTop, trCld, NULL, false, false, true, IT_DASH, IT_LINEBRKAFTER);

					nDeviations++;


					//единичные не берём

					{
						CognateList cl(trOut, inMult, trCld);

						for (itGroup.TryEnterGroup(); cGroup; cGroup = itGroup.Next())
						{
							if (cGroup->comparanda[iColDiff].formOrig)
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
					OutputSoundsHeader(cOther, trOut, inMult, false, false, true, IT_DASH, IT_HORLINE);



					trCld->Add(cOther->comparanda[iColDiff].Text(), IT_SQRBRK | IT_SPACE);
					trCld->Add(L"в отклоняющемся ряду", IT_COLUMN | IT_SPACE);
					OutputSoundsHeader(cOther, trCld, NULL, false, false, true, IT_DASH, IT_LINEBRKAFTER);



					{
						bool isRows = false;
						CognateList cl(trOut, inMult, trCld);
						for (itOther.TryEnterGroup(); cOther; cOther = itOther.Next())
						{
							if (cOther->comparanda[iColDiff].formOrig)
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

void Comparison::OutputCorrespondencesWithMaterial(Condition* cnd, InfoTree* trOut, bool doMakeTableForSingles = false)//нельзя тут повторять
{
	LPTSTR word;
	Sound* sound;

	InfoNode* inCnd, *inMult, *inOnce, *inMultList;

	inCnd = trOut->Add(cnd->title, IT_COLUMN | IT_LINEBRKBEFORE, NULL, false, cnd);

	if (!doMakeTableForSingles)
	{
		inMultList = trOut->Add(L"Оглавление (только неединичные соответствия)", IT_COLUMN | IT_EMPTYLINEBEFORE | IT_LINEBRKAFTER, inCnd);
		inMult = trOut->Add(L"Материал — неединичные соответствия", IT_COLUMN | IT_EMPTYLINEBEFORE | IT_LINEBRKAFTER, inCnd);
		OutputLanguageHeader(trOut, inMult, false);
		inOnce = trOut->Add(L"Материал — единичные соответствия", IT_COLUMN | IT_EMPTYLINEBEFORE | IT_LINEBRKAFTER, inCnd);
		OutputLanguageHeader(trOut, inOnce, false);
	}
	else
	{
		//		inMultList = trOut->Add(NULL, IT_COLUMN | IT_EMPTYLINEBEFORE|IT_LINEBRKAFTER , inCnd);
		inMult = trOut->Add(NULL, IT_COLUMN | IT_HORLINE, inCnd);
		//trOut->Add(NULL, IT_HORLINE, inMult);
	}

	InfoNode* inTo = inOnce;

	Correspondence* c;
	for (CorrespondenceTree::Iterator it(&tCorrespondences); c = it.Next();)
	{
		if (it.IsStartOfGroup() || doMakeTableForSingles)
		{
			if (!doMakeTableForSingles)
				OutputSoundsHeader(c, trOut, inMultList, false, false, false, IT_DASH, IT_LINEBRK);

			inTo = inMult;
			OutputSoundsHeader(c, trOut, inTo, false, true, false, IT_TAB, IT_HORLINE);
		}


		for (int iCol = 0; iCol < nDicts; iCol++)
		{
			trOut->Add(c->comparanda[iCol].formOrig, IT_TAB, inTo);
			trOut->Add(c->comparanda[iCol].translation, IT_MARRQUOTES | IT_TAB, inTo);
		}

		if (it.IsEndOfGroup() || doMakeTableForSingles)
		{
			trOut->Add(NULL, IT_HORLINE, inTo);
			inTo = inOnce;
		}
	}
	if (!doMakeTableForSingles)
	{
		trOut->Add(NULL, IT_HORLINE, inOnce);
		trOut->Add(NULL, IT_SECTIONBRK, inOnce);
	}
}

void Comparison::OutputReconstructedWords(Comparison* cmp, InfoTree* trOut)//нельзя тут повторять
{
	InfoNode* inMult = trOut->Add(L"Реконструкции", IT_COLUMN | IT_EMPTYLINEBEFORE | IT_LINEBRKAFTER, NULL);
	OutputLanguageHeader(trOut, inMult, true);

	for (int iRow = 0; iRow < cmp->nCorresp; iRow++)
	{
		//if (formIPACur = cmp[0].corresps[iRow].comparanda[iCol].formIPA)
		for (int iCol = 0; iCol < nDicts; iCol++)
		{
			if (cmp[0].corresps[iRow].comparanda[0].formIPA)//реконструкция
			{
				trOut->Add(cmp[0].corresps[iRow].comparanda[iCol].formOrig, IT_TAB | (IT_ASTERISK*(iCol == 0)), inMult);
				trOut->Add(cmp[0].corresps[iRow].comparanda[iCol].translation, IT_MARRQUOTES | IT_TAB, inMult);
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
	inMult = trOut->Add(NULL, IT_COLUMN | IT_HORLINE, inCnd);

	InfoNode* inTo = inMult;

	Correspondence* c;
	for (CorrespondenceTree::Iterator it(&tCorrespondences); c = it.Next();)
	{
		if (it.IsStartOfGroup() || c->rankAllSoundsSame >= 5)
			OutputSoundsHeader(c, trOut, inMultList, true, false, false, IT_DASH, IT_LINEBRK);

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
		trOut->Add(dictinfos[iCol].name, IT_TAB, inCnd);
	inMtx = trOut->Add(NULL, IT_LINEBRK, inCnd);

	for (int iRow = 0; iRow < nDicts; iRow++)
	{
		trOut->Add(dictinfos[iRow].name, IT_TAB, inMtx);

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
