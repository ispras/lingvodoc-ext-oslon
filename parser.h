#pragma once

#define PARSER_SKIPNEWLINE		0x1
#define PARSER_NONNULLEND		0x2

class Parser
{
	LPTSTR 	pos;
	LPTSTR 	old;
	LPTSTR 	text;
	LPTSTR 	seps;
	int		flags;
public:
	Parser(LPTSTR _text, LPTSTR _seps, int _flags = 0)
	  //: text(_text)/*ничего не делает*/, flags(_flags), seps(_seps)
	{
		flags = _flags;
		seps = _seps;
		old = NULL;
		pos = text = _text;
	}
	bool IsItemEmpty()
	{
		return *old== '\0';
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
	
	LPTSTR Next() //пишет нули в исходную строку
	{
		if (!(flags & PARSER_NONNULLEND))
		{
			if (*pos == '\0')
				return NULL;
		}
		
		for (old = pos; ; pos++)
		{
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
						return old;
					}
					else
					{
						*pos = '\0';
						pos++;
						return old;
					}
				}
			}
			while (seps[iSep]);
EndSepCycle:
			;
		}
		return NULL;
	}
};

