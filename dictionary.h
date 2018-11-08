#include "parser.h"
#include "ipa.h"

class WordForm : public BNode
{
public:
	LPTSTR	formIPA;
	LPTSTR	formOrig;
	WordForm(LPTSTR _formIPA, LPTSTR _formOrig)
	{
		formOrig = _formOrig;
		formIPA = _formIPA;
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
		return wcscmp(nd1->formIPA, nd2->formIPA);
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
///////////////////////////////////////////////////////////////////////////////////////////////////
enum
{
	RT_LAT,
	RT_CYR,
	RT_COUNT,
	RT_NONE = -1
};

///////////////////////////////////////////////////////////////////////////////////////////////////
class Dictionary
{
public:
	WordFormTree		trWordForms;
	Pool<WordForm>		pWordForms; //(500) — нельзя в старом C++
	Pool<TCHAR>			pString;
	IPA*				ipa;
	Replacer			replacers[RT_COUNT];
	int					iReplacer;
public:
	Dictionary() : pString(1000), pWordForms(1000)
	{
		ipa = new IPA;
		iReplacer = RT_NONE;
		replacers[RT_LAT].Set(ipa, L"лат");
		replacers[RT_LAT].AddRules(_tblReplaceLat);
		replacers[RT_CYR].Set(ipa, L"кир");
		replacers[RT_CYR].AddRules(_tblReplaceCyr);
	}
	~Dictionary()
	{
		delete ipa;
	}

	bool ReplaceSymbols(LPTSTR bIn, LPTSTR bOut)//, LPTSTR lang = NULL)
	{
		if (iReplacer == RT_NONE)
		{
			wcscpy(bOut, bIn);
			return false;
		}

		Replacer* rr = &replacers[iReplacer];

		return rr->Convert(bIn, bOut);
	}


	void GuessReplacer(LPTSTR word)
	{
		if (word[0])
		{
			for (int i = RT_COUNT - 1; i >= 0; i--)//начинаем с кир.
			{
				for (TCHAR* ch = word; *ch; ch++)
				{
					if (replacers[i].IsCharInTable(*ch))
					{
						iReplacer = i;
						return;
					}
				}
			}
		}
		iReplacer = RT_LAT;
	}

	void AddWordList(LPTSTR sIn)
	{
		Parser parser(sIn, L"\r\n", PARSER_SKIPNEWLINE);
		LPTSTR wordIPA, wordOrig;

		TCHAR buf[200];

		while (wordOrig = parser.Next())
		{
			if (iReplacer == RT_NONE)
				GuessReplacer(wordOrig);

			ReplaceSymbols(wordOrig, buf);

			wordOrig = pString.New(wordOrig, wcslen(wordOrig) + 1);
			wordIPA = pString.New(buf, wcslen(buf) + 1);

			WordForm* wfNew = new (pWordForms.New()) WordForm(wordIPA, wordOrig);

			//создаются дубли-сироты этих объектов, если уже в дереве есть

			ipa->SubmitWordForm(wordIPA);

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

		SoundTable::Iterator* it = ipa->Iterator(iClass);
		while (sound = it->Next())
		{
			int flags = IT_TAB;
			if (sound->RowNumber(FT_MANNER) > iRowPrev)
			{
				flags |= IT_LINEBRKBEFORE;
				iColPrev = -1;
			}

			for (int i = iColPrev + 1; i < sound->RowNumber(FT_PLACE); i++)
			{
				trOut->Add(L"", flags);//, ndW);
				flags = IT_TAB;
			}

			ndSound = trOut->Add(sound->Symbol, flags);//, ndW);

			iColPrev = sound->RowNumber(FT_PLACE);
			iRowPrev = sound->RowNumber(FT_MANNER);
		}
		it->Done();
	}

	void BuildDistributionLists(InfoNode** ndRoot, InfoTree* trOut)
	{
		Query qry;

		qry.AddCondition(L"Г", L"#", NULL, 0, L"в начале");
		qry.AddCondition(L"Г", L"ГУБ", NULL, QF_OBJECTONLYONCE, L"после губных");
		qry.AddCondition(L"Г", L"ЗУБ", NULL, QF_OBJECTONLYONCE, L"после зубных");
		qry.AddCondition(L"Г", L"ПАЛ", NULL, QF_OBJECTONLYONCE, L"после палатальных");
		qry.AddCondition(L"Г", L"ЗЯЗ", NULL, QF_OBJECTONLYONCE, L"после заднеязычных");
		qry.AddCondition(L"Г", L"ЛАР", NULL, QF_OBJECTONLYONCE, L"после ларингальных");

		qry.AddCondition(L"Г", NULL, L"ЗУБ", QF_OBJECTONLYONCE, L"перед зубными");

		qry.AddCondition(L"Г", NULL, L"ПАЛ", QF_OBJECTONLYONCE, L"перед палатальными");
		qry.AddCondition(L"Г", NULL, L"ЗЯЗ", QF_OBJECTONLYONCE, L"перед заднеязычными");
		qry.AddCondition(L"Г", NULL, L"ЛАР", QF_OBJECTONLYONCE, L"перед ларингальными");

		qry.AddCondition(L"С", L"#", NULL, 0, L"в начале");
		qry.AddCondition(L"С", NULL, L"ПЕР", QF_OBJECTONLYONCE | QF_CONTEXTONLYONCE, L"перед передними");
		qry.AddCondition(L"С", NULL, L"ЦНТ", QF_OBJECTONLYONCE | QF_CONTEXTONLYONCE, L"перед центральными");
		qry.AddCondition(L"С", NULL, L"ЗАД", QF_OBJECTONLYONCE | QF_CONTEXTONLYONCE, L"перед задними");

		qry.AddCondition(L"С", NULL, L"ОГУ", QF_OBJECTONLYONCE | QF_CONTEXTONLYONCE, L"перед огубленными");

		qry.SetIPA(ipa);//временно, но, честно, пока не знаю, как лучше

		Sound* sdThis;

		for (int iClass = FT_VOWEL; ; iClass = FT_CONSONANT)
		{
			SoundTable::Iterator* it = ipa->Iterator(iClass);
			while (sdThis = it->Next())
			{
				InfoNode* ndThisSound = trOut->Add(sdThis->Symbol, IT_IDENT | IT_SQRBRK | IT_COLUMN | IT_LINEBRKBEFORE, ndRoot[iClass]);

				for (Condition* cnd = qry.FirstCondition(); cnd; cnd = qry.NextCondition())
				{
					if (iClass == cnd->sgThis.feature[FT_CLASS])
						trOut->Add(cnd->title, IT_COLUMN | IT_LINEBRKBEFORE | IT_IDENT, ndThisSound, false, cnd);
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
			Segmentizer sgmntzr(ipa, word->formIPA);

			qry.SetSegmentizer(&sgmntzr);//вызовет ResetConditions

			while (sdCur = sgmntzr.GetNext())
			{
				for (Condition* cnd = qry.FirstCondition(); cnd; cnd = qry.NextCondition())
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
									int fAfterOrig;
									bool isDiff = lstrcmp(word->formOrig, word->formIPA);
									if (isDiff)
										fAfterOrig = IT_SPACE;
									else
										fAfterOrig = IT_COMMA | IT_SPACE;

									trOut->Add(word->formOrig, IT_SPACE | fAfterOrig, ndIter);

									if (isDiff)
										trOut->Add(word->formIPA, IT_SQRBRK | IT_COMMA | IT_SPACE, ndIter);
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