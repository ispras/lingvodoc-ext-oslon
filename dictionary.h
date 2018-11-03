class WordForm : public BNode
{
public:
	LPTSTR	form;
	WordForm(LPTSTR _form)
	{
		form = _form;
	}
/*
	Word	wName;
	WordForm(wchar_t* data)
	{
		wName.posUC = StoreString(data);
		wName.size = wcslen(data) * sizeof(wchar_t);
	}
	~WordForm()
	{
	}
*/
};

class WordFormTree : public BTree
{
public:

	WordFormTree()
	{
		//int ___i=0xabcd;
	}

	int CompareNodes(BNode* _nd1, BNode* _nd2, void*_)
	{
		WordForm* nd1 = (WordForm*)_nd1;//поэтому надо шаблонно!!
		WordForm* nd2 = (WordForm*)_nd2;
		//надо свой порядок вести, см. в Вербе
		return wcscmp(nd1->form, nd2->form);
/*
		int sz = nd1->wName.size;
		
		if (sz > nd2->wName.size)
			sz = nd2->wName.size;

		int dir = memcmp(nd1->wName.pos, nd2->wName.pos, sz);
		
		if (dir == 0)
		{
			if (nd2->wName.size > nd1->wName.size)
				dir = -1;
			else if (nd2->wName.size < nd1->wName.size)
				dir = 1;
		}
		return dir;
		*/
	}
};

class Dictionary
{
public:
	WordFormTree		trWordForms;
	Pool<WordForm>		pWordForms; //(500) — нельзя в старом C++
	IPA*				ipa;

public:
	Dictionary()
	{
		ipa = new IPA;
	}
	~Dictionary()
	{
		delete ipa;
	}

	void AddWordList(LPTSTR sIn)
	{
		Parser parser(sIn, L"\r\n", PARSER_SKIPNEWLINE);
		LPTSTR word;

		while (word = parser.Next())
		{
			WordForm* wfNew = new (pWordForms.New()) WordForm(word);
			//буфера пока нет, весь текст остаётся в изначальной памяти, в к-рую суём нули и подстановки
			//создаются дубли-сироты этих объектов, если уже в дереве есть
			//if (!trWordForms.Add(wfNew))
			ipa->SubmitWordForm(word);
			
			trWordForms.Add(wfNew);
		}
		ipa->EndSubmitWordForms();
	}
/*
	bool IsVowel(LPTSTR symbol)
	{
		return wcsstr(ipaVowels, symbol);
	}
*/
	void BuildIPATable(int iClass, InfoTree* trOut)
	{
		SoundTable::Sound* sound, *soundPrev = NULL;
		
		InfoNode* ndSound = NULL;
		int iColPrev = -1, iRowPrev = 0;
/*
		InfoTree* trOut;
		switch (iClass)
		{
		case FT_CONSONANT:
			trOut = &trIPAConsonants;
			break;
		case FT_VOWEL:
			trOut = &trIPAVowels;
			break;
		case FT_UNKNOWNSOUND:
			trOut = &trIPANotFound;
			break;
		}	
*/	
		SoundTable::Iterator* it = ipa->Iterator(iClass);
		while (sound = it->Next())
		{	
			int flags = 0;//IT_TAB;
			if (sound->RowNumber(FT_MANNER) > iRowPrev)
			{
				flags = IT_LINEBRKBEFORE;
				iColPrev = -1;
			}

			for (int i = iColPrev + 1; i < sound->RowNumber(FT_PLACE); i++)
			{
				trOut->Add(NULL, L"", flags);//, ndW);
				flags = 0;//IT_TAB;
			}

			ndSound = trOut->Add(NULL, sound->Symbol, flags);//, ndW);

			iColPrev = sound->RowNumber(FT_PLACE);			
			iRowPrev = sound->RowNumber(FT_MANNER);
		}
		it->Done();
	}

	void BuildDistributionLists(InfoNode** ndRoot, InfoTree* trOut)
	{
		Query qry;

		qry.AddCondition(QF_FIRSTINWORD, L"???", FT_VOWEL, L"в начале");
		qry.AddCondition(QF_RIGHTAFTER|QF_OBJECTONLYONCE,  L"ГУБ", FT_VOWEL, L"после губных");
		qry.AddCondition(QF_RIGHTAFTER|QF_OBJECTONLYONCE,  L"ЗУБ", FT_VOWEL, L"после зубных");
		qry.AddCondition(QF_RIGHTAFTER|QF_OBJECTONLYONCE,  L"ПАЛ", FT_VOWEL, L"после палатальных");
		qry.AddCondition(QF_RIGHTAFTER|QF_OBJECTONLYONCE,  L"ЗЯЗ", FT_VOWEL, L"после заднеязычных");
		qry.AddCondition(QF_RIGHTAFTER|QF_OBJECTONLYONCE,  L"ЛАР", FT_VOWEL, L"после ларингальных");
		qry.AddCondition(QF_RIGHTBEFORE|QF_OBJECTONLYONCE, L"ГУБ", FT_VOWEL, L"перед губными");
		qry.AddCondition(QF_RIGHTBEFORE|QF_OBJECTONLYONCE, L"ЗУБ", FT_VOWEL, L"перед зубными");
		qry.AddCondition(QF_RIGHTBEFORE|QF_OBJECTONLYONCE, L"ПАЛ", FT_VOWEL, L"перед палатальными");
		qry.AddCondition(QF_RIGHTBEFORE|QF_OBJECTONLYONCE, L"ЗЯЗ", FT_VOWEL, L"перед заднеязычными");
		qry.AddCondition(QF_RIGHTBEFORE|QF_OBJECTONLYONCE, L"ЛАР", FT_VOWEL, L"перед ларингальными");
		
		qry.AddCondition(QF_FIRSTINWORD, L"???", FT_CONSONANT, L"в начале");
		qry.AddCondition(QF_RIGHTBEFORE|QF_CONTEXTONLYONCE, L"ПЕР", FT_CONSONANT, L"перед передними");
		qry.AddCondition(QF_RIGHTBEFORE|QF_CONTEXTONLYONCE, L"ЦНТ", FT_CONSONANT, L"перед центральными");
		qry.AddCondition(QF_RIGHTBEFORE|QF_CONTEXTONLYONCE, L"ЗАД", FT_CONSONANT, L"перед задними");
		qry.AddCondition(QF_RIGHTBEFORE|QF_CONTEXTONLYONCE, L"ОГУ", FT_CONSONANT, L"перед огубленными");

		SoundTable::Sound* sdThis;
		
		for (int iClass = FT_VOWEL; ; iClass = FT_CONSONANT)
		{
			SoundTable::Iterator* it = ipa->Iterator(iClass);
			while (sdThis = it->Next())
			{	
				InfoNode* ndThisSound = trOut->Add(sdThis->Symbol, NULL, IT_IDENT|IT_SQRBRK|IT_COLUMN|IT_LINEBRKBEFORE, ndRoot[iClass]);

				for (Query::Condition* cnd = qry.FirstCondition(); cnd; cnd = qry.NextCondition())
				{
					if (iClass == cnd->iClass)
						trOut->Add(cnd->title, NULL, IT_COLUMN|IT_LINEBRKBEFORE|IT_IDENT, ndThisSound, false, cnd);
				}
			
				sdThis->dataExtra = ndThisSound;
			}
			it->Done();
			if (iClass == FT_CONSONANT)
				break;
		}
			

		WordForm* word;
		
		for (BTree::Walker w(&trWordForms); word = (WordForm*)w.Next();)
		{
			SoundTable::Sound* sdCur, *sdNext = NULL, *sdPrev = NULL;
			Segmentizer sgmntzr(ipa, word->form);
			
			qry.SetSegmentizer(&sgmntzr);//вызовет ResetConditions

			while (sdCur = sgmntzr.GetNext())
			{
				for (Query::Condition* cnd = qry.FirstCondition(); cnd; cnd = qry.NextCondition())
				{
					if (qry.CheckCondition())
					{
						InfoNode* ndSoundInOutput = (InfoNode*)sdCur->dataExtra;
						if (ndSoundInOutput)
						{
							for (InfoNode* ndIter = ndSoundInOutput->chldFirst; ndIter; ndIter = ndIter->next)
							{
								if (ndIter->dataExtra == cnd)
								{
									trOut->Add(NULL, word->form, IT_COMMA|IT_SPACE, ndIter);
									break;
								}
							}
						}
						//break
					}
				}
			}
		}
	}	
};