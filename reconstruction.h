class Reconstruction : public OwnNew
{
	IPA* ipa;
public:
	Query qry;
	Comparison*	comparisons;
	int			nComparisons;

	Reconstruction() //для массива
	{
		//		cmp = NULL;
		//		ipa = NULL;
	}
	Reconstruction(int nCols, int nRows, LPTSTR bufIn = NULL, int begCols = 0, int nCallsAll = 0)
	{
		ipa = new IPA(true);

		nComparisons = 9;
		comparisons = new Comparison[nComparisons];

		for (int i = 0; i < nComparisons; i++)
		{
			new (&comparisons[i]) Comparison(nRows, nCols);
			if (bufIn)
				comparisons[i].AddCognateList(bufIn, false, begCols, nCols, nCallsAll);
			else
				comparisons[i].InitEmpty();
		}

		comparisons[0].condition = qry.AddCondition(L"Г", L"#", NULL, QF_ITERATE, L"Соответствия по начальному гласному");
		comparisons[1].condition = qry.AddCondition(L"С", L"#", NULL, QF_ITERATE, L"Соответствия по начальному согласному");
		comparisons[2].condition = qry.AddCondition(L"Г", L"С", NULL, QF_ITERATE, L"Соответствия по гласному 1-го слога после согласного", 0, 1);
		comparisons[3].condition = qry.AddCondition(L"С", L"Г", NULL, QF_ITERATE, L"Соответствия по согласному после гласного 1-го слога", 0, 1);
		comparisons[4].condition = qry.AddCondition(L"Г", L"С", NULL, QF_ITERATE, L"Соответствия по гласному 2-го слога", 0, 2);
		comparisons[5].condition = qry.AddCondition(L"С", L"Г", NULL, QF_ITERATE, L"Соответствия по согласному после гласного 2-го слога", 0, 2);
		comparisons[6].condition = qry.AddCondition(L"Г", L"С", NULL, QF_ITERATE, L"Соответствия по гласному 3-го слога", 0, 3);
		comparisons[7].condition = qry.AddCondition(L"С", L"Г", NULL, QF_ITERATE, L"Соответствия по согласному после гласного 3-го слога", 0, 3);
		comparisons[8].condition = qry.AddCondition(L"Г", L"С", NULL, QF_ITERATE, L"Соответствия по гласному 4-го слога", 0, 4);

	}
	~Reconstruction()
	{
		delete ipa;
		delete[] comparisons;
	}
	int Reconstruct()
	{
		int nCols;
		for (int i = 0; i < nComparisons; i++)
			comparisons[i].Process(comparisons[i].condition, false, true);
		//comparisons[i].Process(comparisons[i].condition, true, true);
		for (int i = 0; i < nComparisons; i++)
			nCols = ReconstructSounds(i);

		ReconstructWords();

		return nCols;
	}
	void ReconstructWord(int iRow, Comparandum* cReconstr)//, LPTSTR formIPACur)
	{
		TCHAR _bOut[1000];
		LPTSTR bOut = _bOut;

		wcscpy(bOut, L"");

		//bool wasSomething = false;
		for (int i = 0; i < nComparisons; i++)
		{
			LPTSTR txt = NULL;
			Correspondence* crsp = &comparisons[i].corresps[iRow];
			//if (crsp->IsHeadOfGroup()||crsp->IsInGroup())
			//{

			if (crsp->IsInGroup())
				crsp = crsp->crspMain;

			txt = crsp->comparanda[0].Text();
			if (!txt)
			{
				for (int iCol = 1; iCol < comparisons[0].nDicts; iCol++)
				{
					comparisons[i].condition->Reset();
					switch (comparisons[i].condition->GetFirstMatchingFragment(
						ipa,
						NULL,//&sd,
						comparisons[i].corresps[iRow].comparanda[iCol].formIPA,
						NULL))//chr))
					{
					case ST_ERROR:
					case ST_NONE:
						break;
					default:
						txt = L"?";
						goto PosFound;
					}
				}

			}
		PosFound:
			if (txt)
			{
				wcscpy(bOut, txt);
				bOut += wcslen(txt);
			}
		}

		cReconstr->formIPA = comparisons[0].llDicts.first->StoreString(_bOut);
		cReconstr->formOrig = comparisons[0].llDicts.first->StoreString(_bOut);
		cReconstr->isReconstructed = true;
	}
	void ReconstructWords()
	{
		for (int iRow = 0; iRow < comparisons[0].nRowsAll; iRow++)
		{
			ReconstructWord(iRow, &comparisons[0].corresps[iRow].comparanda[0]);//, formIPACur);
		}
	}

	int ReconstructSounds(int iCmp)
	{
		Comparison* cmp = &comparisons[iCmp];
		Condition* cnd = cmp->condition;

		int nCols = cmp->AddDictionary(L"ПРАЯЗЫК");

		//for (int iRow = 0; iRow < cmp->nRowsAll; iRow++)
		Correspondence* crsp;
		for (CorrespondenceTree::Iterator it(&cmp->tCorrespondences); crsp = it.Next();)
		{
			if (it.IsStartOfGroup() || crsp->rankAllSoundsSame >= 5)
			{
				//Correspondence* crsp = &cmp->corresps[iRow];
				Comparandum* c = &crsp->comparanda[0];
				switch (crsp->rankAllSoundsSame)
				{
				case 10:
					if (!CopyIfFragment(c, crsp, cmp))
						CopySoundOrFragment(c, &crsp->comparanda[1]);
					break;
				case 5:
					FindMostComplexSoundOrFragment(c, crsp, cmp);
					break;
				case 0:
					if (!CopyIfFragment(c, crsp, cmp))
						FindAverageSoundOrFragment(c, crsp, cmp, cnd);
				}
				crsp->comparanda[0].isReconstructed = true;
			}
			//else
			it.TryExitGroup();
		}

		//ipa->EndSubmitWordForms();

		for (int iClass = FT_VOWEL; iClass <= FT_CONSONANT; iClass++)
		{
			Sound* sd;
			SoundTable::Iterator* it = ipa->Iterator(iClass);
			while (sd = it->Next())
				sd->used = false;
			it->Done();
		}


		return nCols;
	}
	void CopySoundOrFragment(Comparandum* cReconstr, Comparandum* c)
	{
		switch (c->typeOfSegment)
		{
		case ST_SOUND:
		case ST_EMPTYAUTOFILL:
			cReconstr->SetSound(ipa->SubmitWordForm(c->sound->Symbol));
			cReconstr->sound->used = true;
			break;
		case ST_FRAGMENT:
			cReconstr->SetFragment(c->chrFragment);
			cReconstr->sound = ipa->GetSound(c->chrFragment[0]);
			break;
		default:
			;//out(c->typeOfSegment);
		}
	}

	bool CopyIfFragment(Comparandum* cReconstr, Correspondence* crsp, Comparison* cmp)
	{
		for (int iCol = 1; iCol < cmp->nDicts; iCol++)
		{
			if (crsp->comparanda[iCol].typeOfSegment == ST_FRAGMENT)
			{
				CopySoundOrFragment(cReconstr, &crsp->comparanda[iCol]);
				return true;
			}
		}
		return false;
	}

	bool FindMostComplexSoundOrFragment(Comparandum* cReconstr, Correspondence* crsp, Comparison* cmp)
	{
		if (CopyIfFragment(cReconstr, crsp, cmp))
			return true;

		for (int iCol = 1; iCol < cmp->nDicts; iCol++)
		{
			if (crsp->comparanda[iCol].sound->feature[FT_COARTICULATION])
			{
				CopySoundOrFragment(cReconstr, &crsp->comparanda[iCol]);

				break;
			}
		}
		return true;
	}
	bool FindAverageSoundOrFragment(Comparandum* cReconstr, Correspondence* crsp, Comparison* cmp, Condition* cnd)
	{
		int iClass;
		for (iClass = FT_VOWEL; iClass <= FT_CONSONANT; iClass++)
		{
			if (cnd->sgThis.feature[FT_CLASS] == iClass)
				break;
		}

		DistanceMatrix mtx(ipa, 2);


		DistanceTree dt;
		Distance *d;
		Sound* sd;
		SoundTable::Iterator* it = ipa->Iterator(iClass);
		while (sd = it->Next())
		{
			d = dt.New(sd);

			for (int iCol = 1; iCol < cmp->nDicts; iCol++)
			{
				d->dist += mtx.GetDistance(0, 1, &d->cmp, &crsp->comparanda[iCol], 1);
			}
			dt.Add(d);
		}
		it->Done();

		for (BTree::Walker w(&dt); d = (Distance*)w.Next();)
		{
			if (!d->cmp.sound->used)
			{
				d->cmp.sound->used = true;
				cReconstr->SetSound(d->cmp.sound);
				return true;
			}
		}
		cReconstr->sound = NULL;

		return false;
	}
	void CopyColumnFrom(Reconstruction& rcFrom, int iColFrom, int iColTo)
	{
		for (int i = 0; i < nComparisons; i++)
		{
			//временно: тут копируем только из первого (rcFrom.cmp[0]), т.к. только там сидят реконструкции, что надо переделать!!!
			comparisons[i].CopyDictionaryFrom(&rcFrom.comparisons[0], iColFrom, iColTo);
		}
	}
};