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
	//LPTSTR				txtRules[2];
	//Pool<TCHAR>			pString;
	Pool<Rule>			pRules;
	Pool<Condition>		pConditions;

	Replacer() : pConditions(10), pRules(50)
	{
		txtRules = NULL;
		//txtRules[0] = txtRules[1] = NULL;

		int nipaAll = 0xffff;
		int sz = sizeof(Rule*)*nipaAll;
		rules = (Rule**)malloc(sz);//не new, чтоб избежать к-ра в цикле
		memset(rules, 0, sz);
	}
	~Replacer()
	{
		free(rules);
		//if (txtRules[0])
		//	delete txtRules[0];
		//if (txtRules[1])
		//	delete txtRules[1];
		if (txtRules)
			delete txtRules;
		//УТЕЧКА
	}
	void Set(IPA* _ipa, LPTSTR _lang)
	{
		ipa = _ipa;
		lang = _lang;
	}
	Rule* CreateRule(LPTSTR replaceWhat, LPTSTR replaceBy, bool isConditional = false)
	{
		Rule* rule = rules[replaceWhat[0]];
		if (!rule)
		{
			rule = rules[replaceWhat[0]] = new (pRules.New()) Rule;
		}
		else
		{
			Rule* ruleNew = new (pRules.New()) Rule;

			int szNewToReplace = wcslen(replaceWhat);


			Rule *ruleLast = NULL;
			for (Rule* r = rule; r; r = r->nextSame)
			{
				int szOldToReplace = wcslen(r->symbolToReplace);
				if (szNewToReplace >= szOldToReplace || (szNewToReplace == szOldToReplace && isConditional && !r->condition))
				{
					rule->nextSame = r;
					break;
				}

				ruleLast = r;
			}
			if (!ruleLast)
				rules[replaceWhat[0]] = ruleNew;
			else
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
		lstrcpy(txtRules, _txtRules);

		Parser parser(txtRules, L">,|_\r\n", PARSER_SKIPNEWLINE | PARSER_SKIPSPACES | PARSER_SKIPTABS);

		Rule* rule = NULL;
		bool isCond = false;

		LPTSTR word, fPrev, fThis, fNext;
		fPrev = fThis = fNext = NULL;

		while (word = parser.Next())
		{
			switch (parser.Separator())
			{
			case L'>':
				fThis = word;
				break;
			case L'|':
				isCond = true;
				rule = CreateRule(fThis, word, true);
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
					if (!fThis)
						;//ВЫДАТЬ ОШИБКУ!!
					else
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

	bool ___CopyOrReplaceSymbols(Rule* rule, LPTSTR* bIn, LPTSTR *bOut, Segmentizer* sgmntzr = NULL, int level = 0)
	{
		//if (doCondtionFirst && rule->condition)

		if (rule->nextSame)
		{
			if (___CopyOrReplaceSymbols(rule->nextSame, bIn, bOut, sgmntzr, level + 1))
				return true;
		}

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
			return true;
		}
		else if (isSymbolsOK)
		{
			ReplaceSymbols(bIn, bOut, szToReplace, rule->symbolToReplaceBy);
			return true;
		}

		if (level == 0)
			JustCopySymbols(bIn, bOut, 1);

		return false;
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

	int Convert(LPTSTR bInBeg, LPTSTR bOutBeg)//, int szOut)
	{
		//1-й проход
		LPTSTR bInEnd = bInBeg + wcslen(bInBeg);
		LPTSTR bOut = bOutBeg;

		LPTSTR bIn = bInBeg;
		while (*bIn)
		{
			Rule* rule = rules[*bIn];

			if (!rule || rule->condition)
				JustCopySymbols(&bIn, &bOut, 1);
			else
				CopyOrReplaceSymbols(rule, &bIn, &bOut);
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

			if (!rule || !rule->condition)
				JustCopySymbols(&bIn, &bOut, 1);
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
