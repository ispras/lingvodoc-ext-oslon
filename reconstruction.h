class Reconstruction
{
	IPA* ipa;
public:
	Reconstruction()
	{
		ipa = new IPA(true);
	}
	~Reconstruction()
	{
		delete ipa;
	}
	void ReconstructWord(Comparison* cmp, int nCmp, int iRow, Comparandum* cReconstr)//, LPTSTR formIPACur)
	{
		//TCHAR _bIn[1000];
		TCHAR _bOut[1000];
		//LPTSTR bIn = _bIn;
		LPTSTR bOut = _bOut;

		wcscpy(bOut, L"");

		//bool wasSomething = false;
		for (int i = 0; i < nCmp; i++)
		{
			LPTSTR txt = NULL;
			if (cmp[i].corresps[iRow].IsHeadOfGroup() || cmp[i].corresps[iRow].IsInGroup())
			{
				Correspondence* crsp = &cmp[i].corresps[iRow];
				if (crsp->IsInGroup())
					crsp = crsp->crspMain;

				txt = crsp->comparanda[0].Text();

				//wasSomething = true;

				//JustCopySymbols(&bIn, &bOut, cmp[0].corresps[iRow].comparanda[0].Text());
				//break;
			}
			//else if (i > 1 && wasSomething)
			//{
			//	txt = L"?";
			//}
			if (!txt)
			{
				for (int iCol = 1; iCol < cmp[0].nDicts; iCol++)
				{
					//if (cmp[i].corresps[iRow].comparanda[iCol].formIPA) out(cmp[i].corresps[iRow].comparanda[iCol].formIPA);					
										//Sound* sd; TCHAR chr[10];
					cmp[i].condition->Reset();
					switch (cmp[i].condition->GetFirstMatchingFragment(
						ipa,
						NULL,//&sd,
						cmp[i].corresps[iRow].comparanda[iCol].formIPA,
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


		//		for (int i = 0; i < nCmp; i++)
		//		{
					/*
					Sound* sdCur;
					switch (cmpThis->condition->GetFirstMatchingFragment(
										cmpThis->dic.ipa,
										&cReconstr.sound,
										formIPACur,
										cReconstr.chrFragment))
					{
					case ST_ERROR:
						JustCopy(&bIn, &bOut, 1);
						break;
					default:
						ReplaceSymbols(&bIn, &bOut, wcslen(cReconts);
					//case ST_FRAGMENT:
					//	break;
					//case ST_FRAGMENTMAYBE:
					}
					*/



					//		}
					/*
							Segmentizer sgmntzr(ipa, bIn, true);

							Sound* sdCur;
							while (sdCur = sgmntzr.GetNext())
							{
								bIn = sgmntzr.CurrentPos();

								if (!rule->condition)
								{
									JustCopySymbols(&bIn, &bOut, 1);
								}
								else
								{
									//у нас пока с условием может быть замена только ОДНОГО знака
									CopyOrReplaceSymbols(rule, &bIn, &bOut, &sgmntzr);
								}
							}
							//*bOut = L'\0';
					*/


		cReconstr->formIPA = cmp[0].dic.StoreString(_bOut);
		cReconstr->formOrig = cmp[0].dic.StoreString(_bOut);

		//cReconstr->translation = L"хобана";
	}
	void ReconstructWords(Comparison* cmp, int nCmp, Query& qry)
	{
		for (int iRow = 0; iRow < cmp[0].nCorresp; iRow++)
		{
			//			LPTSTR formIPACur;
			//			int iCol;
			//			for (int iCol = 1; iCol < cmp[0].nDicts; iCol++)
			//			{
			//				if (formIPACur = cmp[0].corresps[iRow].comparanda[iCol].formIPA)
			//					break;
			//			}

			ReconstructWord(cmp, nCmp, iRow, &cmp[0].corresps[iRow].comparanda[0]);//, formIPACur);
/*
			for (int i = 0; i < nCmp; i++)
			{
				if (cmp[i].corresps[iRow].IsInGroup())
				{
					ReconstructWord(&cmp[i], cmp, nCmp, &cmp[0].corresps[iRow].comparanda[0], formIPACur);
					break;
				}
			}
*/

/*
			for (int i = 0; i < nCmp; i++)
			{
				Correspondence* crsp;
				for (CorrespondenceTree::Iterator it(&cmp[i].tCorrespondences); crsp = it.Next();)
				{
					if (!it.IsStartOfGroup())
					{
						it.TryExitGroup();
						continue;
					}
					if (!wcscmp(cmp[i].corresps[iRow].comparanda[iCol].formIPA, formIPACur))
					{

					}
				}
			}
*/
		}
	}

	int ReconstructSounds(Comparison* cmp, Condition* cnd)
	{
		int nCols = cmp->AddDictionary(L"ПРАЯЗЫК", 0);

		//for (int iRow = 0; iRow < cmp->nCorresp; iRow++)
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
			;	//out(c->typeOfSegment);
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
};