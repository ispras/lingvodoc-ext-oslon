#pragma once

#define PARSER_SKIPNEWLINE		0x1
#define PARSER_NONNULLEND		0x2
#define PARSER_CHECKSIZE		0x4

class Parser
{
	LPTSTR 	pos;
	LPTSTR 	old;
	LPTSTR 	text;
	LPTSTR 	seps;
	int 	size;
	int		flags;
	TCHAR	separatorLast;
	bool	isPastEnd;
public:
	Parser(LPTSTR _text, LPTSTR _seps, int _flags = 0, int _size = 0)
		//: text(_text)/*ничего не делает*/, flags(_flags), seps(_seps)
	{
		separatorLast = L'\0';
		flags = _flags;
		seps = _seps;
		isPastEnd = false;
		old = NULL;
		pos = text = _text;
		size = _size;
	}
	void ResetText(LPTSTR _text)
	{
		isPastEnd = false;
		old = NULL;
		pos = text = _text;
	}
	bool IsItemEmpty()
	{
		return *old == L'\0';
	}
	TCHAR Separator()
	{
		return separatorLast;
	}
	LPTSTR Current()
	{
		return old;
	}
	bool IsItemFirstInLine()
	{
		//if (!old)//должно лететь
		//	return false;
		//else
		if (old == text)
			return true;
		else if (old[-1] == L'\n')
			return true;
		else
			return false;
	}
	int LengthOfCurrentWord()
	{
		return pos - old - 1;
	}
	LPTSTR Next() //пишет нули в исходную строку
	{
		if (isPastEnd)
			return NULL;

		if (!(flags & PARSER_NONNULLEND))
		{
			if (*pos == L'\0')
			{
				isPastEnd = true;
				return NULL;
			}
		}

		for (old = pos; ; pos++)
		{

			if ((flags & PARSER_NONNULLEND) && (flags & PARSER_CHECKSIZE))
			{
				if (pos >= text + size)
				{
					isPastEnd = true;
					return NULL;
				}
			}

			int iSep = -1;
			do
			{
				iSep++;
				if (*pos == seps[iSep])
				{
					if ((flags & PARSER_SKIPNEWLINE) && *pos == L'\n')
					{
						//	old = ++pos; //ЭТО НЕ СДЕЛАНО!!
						old = pos + 1;
						goto EndSepCycle;
					}
					else if (!(flags & PARSER_NONNULLEND) && *pos == L'\0')
					{
						separatorLast = *pos;
						pos++;
						isPastEnd = true;
						return old;
					}
					else
					{
						separatorLast = *pos;
						*pos = L'\0';
						pos++;
						return old;
					}
				}
			} while (seps[iSep]);
		EndSepCycle:;
		}
		return NULL;
	}
};

