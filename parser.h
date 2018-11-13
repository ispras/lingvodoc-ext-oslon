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
public:
	Parser(LPTSTR _text, LPTSTR _seps, int _flags = 0, int _size = 0)
		//: text(_text)/*ничего не делает*/, flags(_flags), seps(_seps)
	{
		separatorLast = '\0';
		flags = _flags;
		seps = _seps;
		old = NULL;
		pos = text = _text;
		size = _size;
	}
	bool IsItemEmpty()
	{
		return *old == '\0';
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
		else if (old[-1] == '\n')
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
		if (!(flags & PARSER_NONNULLEND))
		{
			if (*pos == '\0')
				return NULL;
		}

		for (old = pos; ; pos++)
		{

			if ((flags & PARSER_NONNULLEND) && (flags & PARSER_CHECKSIZE))
			{
				if (pos >= text + size)
					return NULL;
			}

			int iSep = -1;
			do
			{
				iSep++;
				if (*pos == seps[iSep])
				{
					if ((flags & PARSER_SKIPNEWLINE) && *pos == '\n')
					{
						//	old = ++pos; //ЭТО НЕ СДЕЛАНО!!
						old = pos + 1;
						goto EndSepCycle;
					}
					else if (!(flags & PARSER_NONNULLEND) && *pos == '\0')
					{
						separatorLast = *pos;
						return old;
					}
					else
					{
						separatorLast = *pos;
						*pos = '\0';
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

