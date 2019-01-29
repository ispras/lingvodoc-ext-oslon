#include "parser.h"
#include "ipa.h"

class WordForm : public BNode
{
public:
	LPTSTR	formIPA;
	LPTSTR	formOrig;
	LPTSTR	wordTranslation;
	WordForm(LPTSTR _formIPA, LPTSTR _formOrig, LPTSTR _wordTranslation)
	{
		formOrig = _formOrig;
		formIPA = _formIPA;
		wordTranslation = _wordTranslation;
	}
};

class WordFormTree : public BTree
{
public:

	int CompareNodes(BNode* _nd1, BNode* _nd2, void*_)
	{
		WordForm* nd1 = (WordForm*)_nd1;//поэтому надо шаблонно!!
		WordForm* nd2 = (WordForm*)_nd2;
		//надо свой порядок вести, см. в Вербе
		return wcscmp(nd1->formIPA, nd2->formIPA);
	}
};
///////////////////////////////////////////////////////////////////////////////////////////////////
enum
{
	RT_LAT,
	RT_CYR,
	RT_COUNT,
	RT_NONE = 100000,
	RT_DEFAULT,
	RT_GUESS
};

///////////////////////////////////////////////////////////////////////////////////////////////////
class DictInfo : public OwnNew//надо это вмонтировать в сам класс Dictionary
{
public:
	int			iReplacer;
	LPTSTR		name;
	DictInfo(LPTSTR _name)
	{
		name = _name;
		iReplacer = RT_NONE;
	}
	DictInfo()
	{
		name = NULL;
		iReplacer = RT_NONE;
	}
};


class Dictionary
{
public:
	WordFormTree		trWordForms;
	Pool<WordForm>		pWordForms; //(500) — нельзя в старом C++
	Pool<TCHAR>			pString;
	IPA*				ipa;
	Replacer			replacers[RT_COUNT];
	int					iReplacer;
	int					nWordForms;
	DictInfo			dictinfo;
public:
	Dictionary() : pString(10000), pWordForms(1000)
	{
		ipa = new IPA;

		nWordForms = 0;

		replacers[RT_LAT].Set(ipa, L"лат");
		replacers[RT_LAT].AddRules(_tblReplaceLat);
		replacers[RT_CYR].Set(ipa, L"кир");
		replacers[RT_CYR].AddRules(_tblReplaceCyr);
	}
	~Dictionary()
	{
		delete ipa;
	}

	int ReplaceSymbols(LPTSTR bIn, LPTSTR bOut/*, int szOut*/, int iReplacerToUse = RT_DEFAULT)//, LPTSTR lang = NULL)
	{
		switch (iReplacerToUse)
		{
		case RT_DEFAULT:
			iReplacerToUse = dictinfo.iReplacer;
			break;
		case RT_NONE:
			wcscpy(bOut, bIn);
			return lstrlen(bOut);
		case RT_GUESS:
			//сделать
			return 0;
		}

		return replacers[iReplacerToUse].Convert(bIn, bOut);//, szOut);
	}


	int GuessReplacer(LPTSTR word)
	{
		if (word[0])
		{
			for (TCHAR* ch = word; *ch; ch++)
			{
				int nMatch = 0;
				int iMatch;
				for (int i = 0; i < RT_COUNT; i++)
				{
					if (replacers[i].IsCharInTable(*ch))
					{
						iMatch = i;
						nMatch++;
					}
				}
				if (nMatch == 1)
					return iMatch;
			}
			return RT_LAT;
		}
		return RT_NONE;
	}

	LPTSTR StoreString(LPTSTR str, int sz = -1)
	{
		if (str[0] == L'\0' || sz == 0)
			return NULL;

		if (sz == -1)
		{
			sz = wcslen(str);
			if (sz == 0)
				return NULL;
		}
		return pString.New(str, sz + 1);
	}

	bool NextCol(int& iCol, int& iRow, int nCols, int nRows)
	{
		if (iCol == nCols - 1)
		{
			(iRow)++;
			if (iRow >= nRows)
				return false;
			iCol = 0;
		}
		else (iCol)++;

		return true;
	}
	void GetOrigIPAAndTranslation(Parser& parser, LPTSTR& wordOrig, LPTSTR& wordIPA, LPTSTR& wordTranslation, DictInfo& dinfo, bool isPhonData, LPTSTR& chrTranscr, LPTSTR& wLen, LPTSTR& wF1, LPTSTR& wF2, LPTSTR& wF3)
	{
		wordOrig = StoreString(parser.Current(), parser.LengthOfCurrentWord());

		TCHAR buf[1000];
		if (!wordOrig)
			wordIPA = NULL;
		else
		{
			if (dinfo.iReplacer == RT_NONE)
				dinfo.iReplacer = GuessReplacer(wordOrig);

			int szIPA = ReplaceSymbols(wordOrig, buf/*, 1000*/, dinfo.iReplacer);
			wordIPA = pString.New(buf, szIPA + 1);
			ipa->SubmitWordForm(wordIPA);
		}

		wordTranslation = parser.Next();
		wordTranslation = StoreString(wordTranslation, parser.LengthOfCurrentWord());

		if (isPhonData)
		{
			chrTranscr = parser.Next();
			chrTranscr = StoreString(chrTranscr, parser.LengthOfCurrentWord());
			wLen = parser.Next();
			wLen = StoreString(wLen, parser.LengthOfCurrentWord());
			wF1 = parser.Next();
			wF1 = StoreString(wF1, parser.LengthOfCurrentWord());
			wF2 = parser.Next();
			wF2 = StoreString(wF2, parser.LengthOfCurrentWord());
			wF3 = parser.Next();
			wF3 = StoreString(wF3, parser.LengthOfCurrentWord());
		}
	}

	void GetDictInfo(Parser& parser, DictInfo& dictinfo)
	{
		LPTSTR wordOrig = StoreString(parser.Current(), parser.LengthOfCurrentWord());
		new (&dictinfo) DictInfo(wordOrig);
		LPTSTR wordAbbrName;
		wordAbbrName = parser.Next();
	}
	void AddWordList(LPTSTR sIn, int nRows)
	{
		Parser parser(sIn, L"\0", PARSER_NONNULLEND);
		LPTSTR wordOrig, wordIPA, wordTranslation, /*надо бы избавиться*/ wchrTranscr = NULL, wLength = NULL, wF1 = NULL, wF2 = NULL, wF3 = NULL;


		int nDicts = 1;
		int iRow = -1, iCol = -1;
		while (parser.Next())
		{
			if (!NextCol(iCol, iRow, nDicts, nRows)) break;

			if (iRow == -1)
			{
				GetDictInfo(parser, dictinfo);
			}
			else
			{
				GetOrigIPAAndTranslation(parser, wordOrig, wordIPA, wordTranslation, dictinfo, false, wchrTranscr, wLength, wF1, wF2, wF3);

				WordForm* wfNew = new (pWordForms.New()) WordForm(wordIPA, wordOrig, wordTranslation); //создаются дубли-сироты этих объектов, если уже в дереве есть
				WordForm* wfFound = (WordForm*)trWordForms.Add(wfNew);
				if (!wfFound)
					nWordForms++;
			}
		}
		ipa->EndSubmitWordForms();
	}
	/*
		void AddWordList_CTAPOE(LPTSTR sIn)
		{
			Parser parser(sIn, L"\r\n", PARSER_SKIPNEWLINE);
			LPTSTR wordIPA, wordOrig;

			TCHAR buf[200];

			while (wordOrig = parser.Next())
			{
				if (iReplacer == RT_NONE)
					iReplacer = GuessReplacer(wordOrig);
				ReplaceSymbols(wordOrig, buf);

				wordOrig = pString.New(wordOrig, wcslen(wordOrig)+1);
				wordIPA = pString.New(buf, wcslen(buf)+1);

				WordForm* wfNew = new (pWordForms.New()) WordForm(wordIPA, wordOrig, NULL);

				//создаются дубли-сироты этих объектов, если уже в дереве есть

				ipa->SubmitWordForm(wordIPA);

				WordForm* wfFound = (WordForm*)trWordForms.Add(wfNew);
				if (!wfFound)
					nWordForms++;
			}
			ipa->EndSubmitWordForms();
		}
	*/
	void BuildIPATable(int iClass, InfoTree* trOut)
	{
		trOut->Add(NULL, IT_HORLINE);

		SoundTable::Sound* sound, *soundPrev = NULL;

		InfoNode* ndSound = NULL;
		int iColPrev = -1, iRowPrev = 0;

		SoundTable::Iterator* it = ipa->Iterator(iClass);
		while (sound = it->Next())
		{
			if (sound->Ordinal(FT_MANNER) > iRowPrev)
			{
				trOut->Add(NULL, IT_LINEBRK);
				iColPrev = -1;
			}

			for (int i = iColPrev + 1; i < sound->Ordinal(FT_PLACE); i++)
			{
				trOut->Add(L"", IT_TAB);
			}

			ndSound = trOut->Add(sound->Symbol, IT_TAB);

			iColPrev = sound->Ordinal(FT_PLACE);
			iRowPrev = sound->Ordinal(FT_MANNER);
		}
		it->Done();

		//trOut->Add(NULL, IT_LINEBRK);
		trOut->Add(NULL, IT_HORLINE);
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

		//qry.DoneAddConditions()

		Sound* sdThis;

		for (int iClass = FT_VOWEL; ; iClass = FT_CONSONANT)
		{
			SoundTable::Iterator* it = ipa->Iterator(iClass);
			while (sdThis = it->Next())
			{
				InfoNode* ndThisSound = trOut->Add(sdThis->Symbol, IT_IDENT | IT_SQRBRK | IT_COLUMN | IT_LINEBRKBEFORE, ndRoot[iClass]);

				for (Condition* cnd = qry.FirstCondition(); cnd; cnd = qry.NextCondition())
				{
					if (cnd->CheckThisFeature(FT_CLASS, iClass, ipa))
						trOut->Add(cnd->title, IT_COLUMN | IT_SPACE | IT_LINEBRKBEFORE | IT_IDENT, ndThisSound, false, cnd);
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
									int isDiff = lstrcmp(word->formOrig, word->formIPA);

									trOut->Add(word->formOrig, IT_SPACE | (IT_COMMA*(!word->wordTranslation && !isDiff)), ndIter);

									if (isDiff)
										trOut->Add(word->formIPA, IT_SQRBRK | IT_SPACE | (IT_COMMA*(!word->wordTranslation)), ndIter);

									if (word->wordTranslation)
										trOut->Add(word->wordTranslation, IT_MARRQUOTES | IT_SPACE | IT_COMMA, ndIter);

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