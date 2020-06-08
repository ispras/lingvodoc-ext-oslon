class Segmentizer : public OwnNew
{
public:
	LPTSTR				pInWord;
	LPTSTR				pOldInWord;
	LPTSTR				text;
	Sound*				sndCurrent;
	Sound*				sndPrevious;
	bool				doSearchModified;
public:
	IPA*				ipa;//на самом деле friend Query?

	Segmentizer(IPA* _ipa, LPTSTR _text, bool _doSearchModified = true)
	{
		doSearchModified = _doSearchModified;
		ipa = _ipa;

		Set(_text);
	}
	void Set(LPTSTR _text)
	{
		text = _text;
		pInWord = pOldInWord = NULL;
		sndCurrent = NULL;
		sndPrevious = NULL;
	}
	LPTSTR CurrentPos()
	{
		return pOldInWord;
	}
	TCHAR Current1Char()
	{
		return pOldInWord[0];
	}
	Sound* Current()
	{
		return sndCurrent;
	}
	Sound* PeekPrevious()
	{
		return sndPrevious;
	}
	Sound* PeekNext()
	{
		return GetNext(true);
	}
	Sound* GetNext(bool doPeek = false)
	{
		Sound* soundBase, *sound;
		LPTSTR pos = pInWord;

		if (!pos)
			pos = text;
		if (!*pos)
			return NULL;

		if (!doPeek)
			pOldInWord = pos;

		TCHAR chr = *pos;

		if (sound = soundBase = ipa->GetSound(chr))
		{
			if (doSearchModified)
			{
				int	feature[FT_NFEATURETYPES];
				TCHAR chrWithMod[9];
				int nPostModifiers = ipa->GetPostModifiers(pos, chrWithMod, feature);
				pos += nPostModifiers;

				if (nPostModifiers)
					sound = ipa->FindModifiedSound(soundBase, feature);
			}
		}
		if (!doPeek)
		{
			sndPrevious = sndCurrent;
			sndCurrent = sound;;
			pInWord = pos + 1;
		}

		return sound;
	}
	bool IsFirst()
	{
		return pOldInWord == text;
	}
	bool IsLast()
	{
		if (!pOldInWord) return false;
		return !*pInWord;
	}
};
