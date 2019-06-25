#include "parser.h"
#include "ipa.h"
#include "phonology.h"

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
	int			nWords;
	int			nFilledSoundCorresp;
	DictInfo(LPTSTR _name)
	{
		nWords = 0;
		nFilledSoundCorresp = 0;
		name = _name;
		iReplacer = RT_NONE;
	}
	DictInfo()
	{
		nWords = 0;
		nFilledSoundCorresp = 0;
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
	Phonology*			phono;
	Replacer			replacers[RT_COUNT];
	int					iReplacer;
	int					nWordForms;
	DictInfo			dictinfo;
public:
	Dictionary() : pString(10000), pWordForms(1000)
	{
		ipa = new IPA;
		phono = new Phonology;

		nWordForms = 0;

		replacers[RT_LAT].Set(ipa, L"лат");
		replacers[RT_LAT].AddRules(_tblReplaceLat);
		replacers[RT_CYR].Set(ipa, L"кир");
		replacers[RT_CYR].AddRules(_tblReplaceCyr);
	}
	~Dictionary()
	{
		delete ipa;
		delete phono;
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
				{
					return iMatch;
				}
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
	LPTSTR TranscribeWord(LPTSTR& wordOrig, DictInfo& dinfo)
	{
		TCHAR buf[1000];
		LPTSTR wordIPA;
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
		return wordIPA;
	}
	void GetOrigIPAAndTranslation(Parser& parser, LPTSTR& wordOrig, LPTSTR& wordIPA, LPTSTR& wordTranslation, DictInfo& dinfo, bool isPhonData, LPTSTR& chrTranscr, LPTSTR& wLen, LPTSTR& wF1, LPTSTR& wF2, LPTSTR& wF3)
	{
		wordOrig = StoreString(parser.Current(), parser.LengthOfCurrentWord());

		wordIPA = TranscribeWord(wordOrig, dinfo);

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

	void GetDictInfo(Parser& parser, DictInfo& dictinfo, LPTSTR wordOrig = NULL)
	{
		if (wordOrig)
			wordOrig = StoreString(wordOrig);
		else
		{
			wordOrig = StoreString(parser.Current(), parser.LengthOfCurrentWord());
			LPTSTR wordAbbrName = parser.Next();
		}

		new (&dictinfo) DictInfo(wordOrig);
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

	void BuildDistributionTables(Query& qry, InfoNode** ndRoot, InfoTree* trOut)
	{
		Sound* sdThis;
		TCHAR buf[200];
		for (int iClass = FT_VOWEL; ; iClass = FT_CONSONANT)
		{
			trOut->Add(NULL, IT_HORLINE, ndRoot[iClass]);
			trOut->Add(NULL, IT_TAB, ndRoot[iClass]);
			trOut->Add(NULL, IT_TAB, ndRoot[iClass]);

			InfoNode* ndSounds = trOut->Add(NULL, IT_TAB, ndRoot[iClass]);

			SoundTable::Iterator* it = ipa->Iterator(iClass);
			while (sdThis = it->Next())
				trOut->Add(sdThis->Symbol, IT_TAB, ndSounds, false, sdThis);
			it->Done();

			for (Condition* cnd = qry.FirstCondition(); cnd; cnd = qry.NextCondition())
			{
				if (cnd->CheckThisFeature(FT_CLASS, iClass, ipa))
				{
					InfoNode* ndDistr = trOut->Add(cnd->AutoTitle(buf, 5), IT_COLUMN | IT_LINEBRKBEFORE, ndRoot[iClass], false, cnd);

					trOut->Add(NULL, IT_TAB, ndDistr);
					for (InfoNode* ndIter = ndSounds->chldFirst; ndIter; ndIter = ndIter->next)
					{
						Sound* sdThis = (Sound*)ndIter->dataExtra;


						//cnd

						WordForm* word;
						TCHAR chrFragment[10];
						Sound* sndGot;
						bool isMet = false;

						for (BTree::Walker w(&trWordForms); word = (WordForm*)w.Next();)
						{
							//такое копирование никуда не годится!
							Condition cndThis(sdThis->Symbol, cnd->sgPrev.txtCondition, cnd->sgNext.txtCondition);

							switch (cndThis.GetFirstMatchingFragment(ipa, &sndGot, word->formIPA, chrFragment))
							{
							case ST_SOUND:
							case ST_FRAGMENT:
								isMet = true;
								goto AddPlusToTable;
							}
						}
					AddPlusToTable:
						if (isMet)
							trOut->Add(L"+", IT_TAB, ndDistr);
						else
							trOut->Add(NULL, IT_TAB, ndDistr);
					}
				}
			}

			trOut->Add(NULL, IT_HORLINE, ndRoot[iClass]);

			if (iClass == FT_CONSONANT)
				break;
		}
	}


	void BuildDistributionLists(Query& qry, InfoNode** ndRoot, InfoTree* trOut)
	{
		Sound* sdThis;
		TCHAR buf[200];
		for (int iClass = FT_VOWEL; ; iClass = FT_CONSONANT)
		{
			SoundTable::Iterator* it = ipa->Iterator(iClass);
			while (sdThis = it->Next())
			{
				InfoNode* ndThisSound = trOut->Add(sdThis->Symbol, IT_IDENT | IT_SQRBRK | IT_COLUMN | IT_LINEBRKBEFORE, ndRoot[iClass]);

				for (Condition* cnd = qry.FirstCondition(); cnd; cnd = qry.NextCondition())
				{
					if (cnd->CheckThisFeature(FT_CLASS, iClass, ipa))
						trOut->Add(/*cnd->title*/ cnd->AutoTitle(buf, 5), IT_COLUMN | IT_LINEBRKBEFORE | IT_IDENT, ndThisSound, false, cnd);
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
			Sound* sdCur, *sdNext = NULL, *sdPrev = NULL;
			Segmentizer sgmntzr(ipa, word->formIPA);

			qry.SetSegmentizer(&sgmntzr);//вызовет ResetConditions

			while (sdCur = sgmntzr.GetNext())
			{
				for (Condition* cnd = qry.FirstCondition(); cnd; cnd = qry.NextCondition())
				{
					if (cnd->Check(&sgmntzr))
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
						//goto NextWord;
					}
				}
			}
		}
	}


};