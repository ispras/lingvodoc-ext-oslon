#include "ipareplacements.h"

class Replacer
{
public:
	class Rule : public OwnNew
	{
	public:
		TCHAR			symbolToReplace[8];
		TCHAR			symbolToReplaceBy[8];
		//void*			dataExtra;
		Rule*			nextSame;
		Condition*		condition;

		Rule()
		{
			condition = NULL;
			nextSame = NULL;
			symbolToReplace[0] = L'\0';
			symbolToReplaceBy[0] = L'\0';
		}
	};
	IPA*				ipa;
	Rule**				rules;
	LPTSTR				lang;
	LPTSTR				txtRules;
	//Pool<TCHAR>			pString;
	Pool<Rule>			pRules;
	Pool<Condition>		pConditions;

	Replacer() : pConditions(10), pRules(50)
	{
		txtRules = NULL;

		int nipaAll = 0xffff;
		int sz = sizeof(Rule*)*nipaAll;
		rules = (Rule**)malloc(sz);//не new, чтоб избежать к-ра в цикле
		memset(rules, 0, sz);
	}
	~Replacer()
	{
		free(rules);
		if (txtRules)
			delete txtRules;
	}
	void Set(IPA* _ipa, LPTSTR _lang)
	{
		ipa = _ipa;
		lang = _lang;
	}
	Rule* CreateRule(LPTSTR replaceWhat, LPTSTR replaceBy)
	{
		Rule* rule = rules[replaceWhat[0]];
		if (!rule)
		{
			rule = rules[replaceWhat[0]] = new (pRules.New()) Rule;
		}
		//if (rule->symbolToReplace[0]) //значит, уже есть, новое надо привесить к нему
		else
		{
			Rule* ruleNew = new (pRules.New()) Rule;

			Rule *ruleLast = rule;
			for (Rule* r = rule->nextSame; r; r = r->nextSame)
				ruleLast = r;
			ruleLast->nextSame = ruleNew;

			rule = ruleNew;
		}

		StrCpyWMax(rule->symbolToReplace, replaceWhat, 8);
		StrCpyWMax(rule->symbolToReplaceBy, replaceBy, 8);
		return rule;
	}
	void AddRules(LPTSTR _txtRules)
	{
		txtRules = new TCHAR[wcslen(_txtRules) + 1];
		//pString.New(_textRules, wcslen(_textRules)+1);
		lstrcpy(txtRules, _txtRules);

		Parser parser(txtRules, L">,|_\r\n", PARSER_SKIPNEWLINE);

		Rule* rule = NULL;
		LPTSTR word;
		LPTSTR fPrev, fThis, fNext;
		fPrev = fThis = fNext = NULL;
		bool isCond = false;
		while (word = parser.Next())
		{
			switch (parser.Separator())
			{
			case L'>':
				fThis = word;
				break;
			case L'|':
				isCond = true;
				rule = CreateRule(fThis, word);
				break;
			case L'_':
				fPrev = word;
				break;
			case L',':
			case L'\r':
			case L'\0'://конец
				if (isCond)
				{
					fNext = word;
					rule->condition = new (pConditions.New()) Condition(fThis, fPrev, fNext, 0, NULL);
				}
				else
				{
					rule = CreateRule(fThis, word);
				}

				rule = NULL;
				fPrev = fThis = fNext = NULL;
				isCond = false;
			}
		}
	}
	bool IsCharInTable(TCHAR chr)
	{
		return !!rules[chr];//.symbolToReplace[0] != L'\0';
	}

	void CopyOrReplaceSymbols(Rule* rule, LPTSTR* bIn, LPTSTR *bOut, Segmentizer* sgmntzr = NULL)
	{
		for (; rule; rule = rule->nextSame)
		{
			int szToReplace = wcslen(rule->symbolToReplace);

			bool isSymbolsOK;

			if (szToReplace <= 1)
				isSymbolsOK = true;
			else
			{
				isSymbolsOK = !wcsncmp(*bIn, rule->symbolToReplace, szToReplace);
			}

			if (isSymbolsOK && rule->condition)
			{
				isSymbolsOK = rule->condition->Check(sgmntzr);
			}

			if (szToReplace == 0)
			{
				JustCopySymbols(bIn, bOut, 1);
				return;
			}
			else if (isSymbolsOK)
			{
				ReplaceSymbols(bIn, bOut, szToReplace, rule->symbolToReplaceBy);
				return;
			}
		}
		JustCopySymbols(bIn, bOut, 1);
	}

	int Convert(LPTSTR bInBeg, LPTSTR bOutBeg, bool __isArchive)//, int szOut)
	{
		//1-й проход
		LPTSTR bInEnd = bInBeg + wcslen(bInBeg);
		LPTSTR bOut = bOutBeg;

		LPTSTR bIn = bInBeg;
		while (*bIn)
		{

			if (__isArchive && *bIn == L'ʃ')
				*bIn = L's';



			Rule* rule = rules[*bIn];
			if (!rule) goto JustCopy;
			if (rule->condition)//пока так, т.к. не сделано ветвление условий
			{
			JustCopy:		JustCopySymbols(&bIn, &bOut, 1);
			}
			else
			{
				CopyOrReplaceSymbols(rule, &bIn, &bOut);
			}
		}
		*bOut = L'\0';
		//2-й проход

		TCHAR buf[2000];//где-то выше проверять длину; длину буфера где-то задавать; или динамически то выделять
		wcscpy(buf, bOutBeg);
		bInBeg = buf;

		Segmentizer sgmntzr(ipa, bInBeg, false);

		bOut = bOutBeg;

		Sound* sdCur;
		while (sdCur = sgmntzr.GetNext())
		{
			Rule* rule = rules[sgmntzr.Current1Char()];
			bIn = sgmntzr.CurrentPos();

			if (!rule) goto JustCopy2;//пока так, т.к. не сделано ветвление условий
			if (!rule->condition)
			{
			JustCopy2:		JustCopySymbols(&bIn, &bOut, 1);
			}
			else
			{
				//у нас пока с условием может быть замена только ОДНОГО знака
				CopyOrReplaceSymbols(rule, &bIn, &bOut, &sgmntzr);
			}
		}
		*bOut = L'\0';

		return bOut - bOutBeg;
	}

};
