#pragma once

#include "stringfunctions.h"
#include "parser.h"
#include "pool.h"
#include "btree.h"
#include "datastructures.h"

#include "ipatables.h"

enum
{
	FT_VOWEL = 1,
	FT_CONSONANT,
	FT_MODIFIER,
	FT_UNKNOWNSOUND,
	FT_NSOUNDCLASSES,
};

enum
{
	FT_CLASS = 0,
	FT_MANNER,
	FT_PLACE,
	FT_COARTICULATION,
	FT_NFEATURETYPES,

	FT_NONE,
	FT_SOUND,
};


class Feature : public BNode
{
public:
	int		iClass;
	int		iType;
	int		value;
	LPTSTR	abbr;
	Feature(LPTSTR _abbr, int _iClass = FT_NONE, int _iType = FT_NONE, int _value = 0)
	{
		iClass = _iClass;
		iType = _iType;
		abbr = _abbr;
		value = _value;
	}
};

int CompareFeaturesAnd(int* f1, int* f2)
{
	for (int iFType = 0; iFType < FT_NFEATURETYPES; iFType++)
	{
		if (f1[iFType] > f2[iFType])
			return 1;
		if (f1[iFType] < f2[iFType])
			return -1;
	}
	return 0;
}

int CompareFeaturesNumberOfDifferences___(int* f1, int* f2)
{
	int nDiff = 0;
	for (int iFType = 0; iFType < FT_NFEATURETYPES; iFType++)
	{
		if (f1[iFType] > f2[iFType])
			nDiff++;
		if (f1[iFType] < f2[iFType])
			nDiff++;
	}
	return nDiff;
}

int CompareFeaturesOr(int* f1, int* f2, int max = FT_NFEATURETYPES - 1)
{
	for (int iFType = 0; iFType <= max; iFType++)
	{
		if (f2[iFType])
		{
			if (!(f1[iFType] & f2[iFType]))
				return 1;
		}
	}
	return 0;
}

class FeatureTree : public BTree
{
public:
	int CompareNodes(BNode* _nd1, BNode* _nd2, void*_)
	{
		Feature* nd1 = (Feature*)_nd1;//поэтому надо шаблонно!!
		Feature* nd2 = (Feature*)_nd2;
		return wcscmp(nd1->abbr, nd2->abbr);
	}
};

#define FT_PREV		0x100
#define FT_SET 		0x200
#define FT_UNSET 	0x400


class SoundTable
{
public:
	class Row;//надо без этого!!!
	class Row : public LinkedElement<Row>, public OwnNew
	{
	public:
		bool		isEmpty;
		int			valBase;
		int			valMod;
		int			i;
		int			iIPA;
		Row(Row* rowPrev = NULL)
		{
			next = prev = NULL;// можно это в LinkedElement()?

			i = 0;
			isEmpty = true;
			valBase = valMod = 0;

			if (rowPrev)
				iIPA = rowPrev->iIPA;
		}
	};

	//сделать SoundRef

	class Sound : public BNode
	{
	public:
		TCHAR			Symbol[8];
		//bool			canExist;
		bool			exists;
		bool			used;
		void*			dataExtra;
		Sound*			nextModified;
		Row*			row[FT_NFEATURETYPES];
		int				feature[FT_NFEATURETYPES];

		Sound(TCHAR* _symbol, int* _feature = NULL)
		{
			if (_symbol)
			{
				lstrcpy(Symbol, _symbol);
				//canExist = true;
			}
			else
			{
				//int i=9;i/=0;
				//canExist = false;
			}

			exists = used = false;
			dataExtra = nextModified = NULL;

			for (int iFType = 0; iFType < FT_NFEATURETYPES; iFType++)
			{
				row[iFType] = NULL;
				if (_feature)
					feature[iFType] = _feature[iFType];
				else
					feature[iFType] = 0;
			}
		}

		Sound(int _code, Row** rows = NULL)//, bool _isPreModifier = false, bool _isPostModifier = false)
		{
			Symbol[0] = _code;
			Symbol[1] = L'\0';

			dataExtra = nextModified = NULL;
			exists = false;

			if (rows)
			{
				for (int iFType = 0; iFType < FT_NFEATURETYPES; iFType++)
				{
					row[iFType] = rows[iFType];
					if (rows[iFType])
					{
						if (iFType == FT_COARTICULATION)
							feature[iFType] = rows[iFType]->valMod;
						else
							feature[iFType] = rows[iFType]->valBase;
					}
					else
						feature[iFType] = 0;
				}
			}
			else
			{
				for (int iFType = 0; iFType < FT_NFEATURETYPES; iFType++)
				{
					row[iFType] = NULL;
					feature[iFType] = 0;
				}
			}
		}
		int Ordinal(int iFType)
		{
			if (!row[iFType])
				return -1;
			return row[iFType]->i;
		}
		int OrdinalInIPA(int iFType)
		{
			if (!row[iFType])
				return -1;
			return row[iFType]->iIPA;
		}
	};

	class SoundTree : public BTree
	{
	public:
		int CompareNodes(BNode* _nd1, BNode* _nd2, void*_)
		{
			return CompareFeaturesAnd(((Sound*)_nd1)->feature, ((Sound*)_nd2)->feature);
		}
	};

public:
	SoundTree		tSounds;

	Sound**			ipaAll;
	int				nipaAll;

	LinkedList<Row> llRows[FT_NFEATURETYPES];
	Row*			rowCurrent[FT_NFEATURETYPES];
	Pool<Row>		pRows;
	Pool<Sound>		pSounds;

	class Iterator
	{
		BTree* tree;
		BTree::Walker walker;
	public:
		Iterator(BTree* _tree) : walker(_tree)
		{
		}
		Sound* Next()
		{
			return (Sound*)walker.Next();
		}
		void Done()
		{
			delete this;
		}
	};

	SoundTable() : pRows(50), pSounds(100)
	{
		ipaAll = NULL;
		for (int iFType = 0; iFType < FT_NFEATURETYPES; iFType++)
			rowCurrent[iFType] = NULL;
	}
	//	~SoundTable()
	//	{
	//	}

	bool InitIPAMemory(Sound*** _pipaAll)
	{
		nipaAll = 0xffff;
		if (!*_pipaAll)
		{
			int sz = sizeof(Sound*)*nipaAll;
			if (!(*_pipaAll = (Sound**)malloc(sz)))
				return false;//не new, чтоб избежать к-ра в цикле

			memset(*_pipaAll, 0, sz);
		}
		ipaAll = *_pipaAll;

		return true;
	}

	Sound* AddUnknownSound(int iChr)
	{
		Sound* sound = ipaAll[iChr] = new (pSounds.New()) Sound(iChr);

		sound->feature[FT_CLASS] = FT_UNKNOWNSOUND;
		sound->feature[FT_MANNER] = iChr;
		sound->exists = true;

		tSounds.Add(sound);

		return sound;
	}
	void ConfirmSound(Sound* sound, int iClass = -1)
	{
		sound->exists = true;
		tSounds.Add(sound);
		if (iClass == FT_MODIFIER)
		{
			Sound* sdMain = ipaAll[sound->Symbol[0]];
			for (int iFType = 0; iFType < FT_NFEATURETYPES; iFType++)
				sound->row[iFType] = sdMain->row[iFType];

			for (int iFType = FT_PLACE; ; iFType = FT_MANNER)
			{
				Row* row;
				Row* rowPrev = NULL;
				for (row = llRows[iFType].first; row; row = row->next)
				{
					if (row->valBase == sound->feature[iFType])
					{
						if (iFType == FT_PLACE)
						{
							if (row->valMod == sound->feature[FT_COARTICULATION])
								goto FoundRow;
							if (row->valMod > sound->feature[FT_COARTICULATION])
								goto AddRow;
						}
						else
							goto FoundRow;
					}
					else if (row->valBase > sound->feature[iFType])
					{
						goto AddRow;
					}

					rowPrev = row;
				}
			AddRow:
				row = new (pRows.New()) Row(rowPrev);
				llRows[iFType].Add(row, rowPrev);

				AddRowValue(iFType, sound->feature[iFType], FT_SET, row);
				AddRowValue(FT_COARTICULATION, sound->feature[FT_COARTICULATION], FT_SET, row);
			FoundRow:
				sound->row[iFType] = row;

				row->isEmpty = false;

				if (iFType == FT_MANNER)
					break;
			}

			sound->nextModified = sdMain->nextModified;
			sdMain->nextModified = sound;
		}

		for (int iFType = FT_PLACE; ; iFType = FT_MANNER)
			//for (int iFType = 0; iFType < FT_NFEATURETYPES; iFType++)
		{
			if (sound->row[iFType])
				sound->row[iFType]->isEmpty = false;
			if (iFType == FT_MANNER)
				break;
		}

	}

	/*
		Sound* Get(int iChr)
		{
			if (!ipaAll[iChr].exists)
				return NULL;
			return &ipaAll[iChr];
		}
	*/
	void AddRowValue(int iFType, int toAdd, int flag, Row* row = NULL)
	{
		/*
		if ((flag & FT_PREV) && rowCurrent[iFType])
		{
			if (rowCurrent[iFType]->prev)
				toAdd = rowCurrent[iFType]->prev->value;
		}
		*/

		if (!row) row = rowCurrent[iFType];

		int* val;
		switch (iFType)
		{
		case FT_COARTICULATION:
			val = &row->valMod;
			break;
		default:
			val = &row->valBase;
		}

		switch (flag & (FT_SET | FT_UNSET))
		{
		case FT_SET:
			*val |= toAdd;
			break;
		case FT_UNSET:
			*val &= ~toAdd;
		}
	}
	Row* GetRow(int iFType, int iRow)
	{
		if (!llRows[iFType].first)
			return NULL;

		int i = 0;
		for (Row* row = llRows[iFType].first; row; row = row->next)
		{
			if (i == iRow)
				return row;
			i++;
		}
		return NULL;
	}
	void UnsetRow(int iFType)
	{
		rowCurrent[iFType] = NULL;//llRows[iFType].first;
	}
	void NextRow(int iFType)
	{
		int iCur;
		if (!llRows[iFType].first)
		{
			iCur = -1;
			goto AddNewRow;
		}
		else if (!rowCurrent[iFType])
		{
			rowCurrent[iFType] = llRows[iFType].first;
		}
		else if (!rowCurrent[iFType]->next)
		{
			iCur = rowCurrent[iFType]->iIPA;
		AddNewRow:
			rowCurrent[iFType] = new (pRows.New()) Row;
			llRows[iFType].Add(rowCurrent[iFType]);
			rowCurrent[iFType]->iIPA = iCur + 1;
		}
		else
		{
			rowCurrent[iFType] = rowCurrent[iFType]->next;
		}
	}
	bool PutSoundInIPATable(int iChr, bool doConfirm)
	{
		if (!IsSoundCodeOK(iChr))
			return false;
		//else if (ipaAll[iChr].canExist)
		//	return false;

		//Sound* sound = 
		//new (&ipaAll[iChr]) Sound(iChr, rowCurrent);//, iRow, iCol, _isPreModifier, _isPostModifier);


		ipaAll[iChr] = new (pSounds.New()) Sound(iChr, rowCurrent);

		if (doConfirm)
		{
			ipaAll[iChr]->exists = true;
			tSounds.Add(ipaAll[iChr]);
		}

		return true;
	}
	void CountRows()
	{
		for (int iFType = 0; iFType < FT_NFEATURETYPES; iFType++)
		{
			int i = 0;
			Row* nxt;
			for (Row* row = llRows[iFType].first; row; row = nxt)
			{
				nxt = row->next;

				if (row->isEmpty)
					llRows[iFType].Delete(row, false);
				else
				{
					row->i = i;
					i++;
				}
			}
		}
	}
	bool IsSoundCodeOK(int iChr)
	{
		return (iChr < nipaAll);
	}
};


///////////////////////////////////////////////////////////////
typedef SoundTable::Sound Sound;
///////////////////////////////////////////////////////////////


class IPA
{
	//	friend Segmentizer;
public://временно вм. friend
	Sound**			ipaAll;
	Pool<Sound>		pSounds;
	Pool<Feature>	pFeatures;
	FeatureTree		tFeatures;
	int 			curFeatureValue[FT_NSOUNDCLASSES];
	SoundTable		tblSounds[FT_NSOUNDCLASSES];
	Pool<TCHAR>		pString;
	LPTSTR			nameFeature[FT_NSOUNDCLASSES];

public:
	LPTSTR tblPostModifiers, tblConsonants, tblVowels;//, tblReplaceLat, tblReplaceCyr;

	IPA(bool doConfirmAllSounds = false) : pString(3000), pSounds(100)
	{
		ipaAll = NULL;

		Feature* f;
		f = new (pFeatures.New()) Feature(L"С", FT_CONSONANT, FT_CLASS, FT_CONSONANT);
		tFeatures.Add(f);
		f = new (pFeatures.New()) Feature(L"Г", FT_VOWEL, FT_CLASS, FT_VOWEL);
		tFeatures.Add(f);


		//копируем пока просто из глобальных литералов, потом как-то иначе будет
		tblPostModifiers = pString.New(_tblPostModifiers, wcslen(_tblPostModifiers) + 1);
		tblConsonants = pString.New(_tblConsonants, wcslen(_tblConsonants) + 1);
		tblVowels = pString.New(_tblVowels, wcslen(_tblVowels) + 1);

		BuildPotentialSoundTable(FT_MODIFIER, tblPostModifiers, FT_COARTICULATION, doConfirmAllSounds);

		BuildPotentialSoundTable(FT_CONSONANT, tblConsonants, FT_PLACE, doConfirmAllSounds);
		BuildPotentialSoundTable(FT_VOWEL, tblVowels, FT_PLACE, doConfirmAllSounds);

		tblSounds[FT_UNKNOWNSOUND].InitIPAMemory(&ipaAll);//плохо, что надо явно вызывать!!!

		nameFeature[FT_CLASS] = L"класс";
		nameFeature[FT_MANNER] = L"способ";
		nameFeature[FT_PLACE] = L"место";
		nameFeature[FT_COARTICULATION] = L"ко-ция";
	}
	~IPA()
	{
		free(ipaAll);
	}
	bool CompareSoundsByText(Sound* s1, Sound* s2)
	{
		return !!wcscmp(s1->Symbol, s2->Symbol);
	}
	Sound* FindModifiedSound(Sound* sndBase, int* feature)
	{
		for (Sound* soundMod = sndBase->nextModified; soundMod; soundMod = soundMod->nextModified)
		{
			Sound sdForCompare(NULL, feature);
			if (!tblSounds[sndBase->feature[FT_CLASS]].tSounds.CompareNodes(soundMod, &sdForCompare, NULL))
				return soundMod;
		}
		return NULL;
	}
	int GetPostModifiers(LPTSTR pInWord, LPTSTR chrWithMod, int* feature)
	{
		Sound* soundBase = ipaAll[*pInWord];
		int nPostModifiers = 0;

		while (true)
		{
			Sound* soundMod = ipaAll[pInWord[1]];

			if (!soundMod)
				break;
			if (soundMod->feature[FT_CLASS] == FT_MODIFIER)
			{
				if (nPostModifiers == 0)//т.е. это первый
				{
					chrWithMod[0] = soundBase->Symbol[0];
					CopyFeatures(feature, soundBase->feature);
				}

				//if (nPostModifiers > ...)//чтоб не было больше 5, например

				nPostModifiers++;

				feature[FT_COARTICULATION] |= soundMod->feature[FT_COARTICULATION];

				chrWithMod[nPostModifiers] = pInWord[1];
			}
			else break;
			pInWord++;
		}

		chrWithMod[nPostModifiers + 1] = L'\0';

		return nPostModifiers;
	}

	Sound* GetSound(TCHAR pChr)
	{
		if (pChr == 0)
			return NULL;
		Sound* sd = ipaAll[pChr];

		//if (!sd)
		//	sd = tblSounds[FT_UNKNOWNSOUND].AddUnknownSound(pChr);
		return sd;
	}

	Sound* GetBaseSound(Sound* sound)
	{
		return GetSound(sound->Symbol[0]);
	}

	Sound* SubmitWordForm(LPTSTR word)//будет меняться подстановками!
	{
		Sound* sdCur;
		for (LPTSTR pInWord = word; *pInWord; pInWord++)
		{
			Sound* soundBase = sdCur = ipaAll[*pInWord];//GetSoundWithReplacement(pInWord);
			TCHAR chr = *pInWord;

			bool isSoundOK = SoundIsInIPA(chr);

			if (!isSoundOK)
			{
				if (!SoundExists(chr))
					tblSounds[FT_UNKNOWNSOUND].AddUnknownSound(chr);
			}


			int	feature[FT_NFEATURETYPES];
			TCHAR chrWithMod[9];
			int nPostModifiers = GetPostModifiers(pInWord, chrWithMod, feature);
			pInWord += nPostModifiers;

			if (isSoundOK)
			{
				int iClass = soundBase->feature[FT_CLASS];
				if (nPostModifiers)
				{
					//SoundTable::Sound sdForSearch(NULL, feature);
					//SoundTable::Sound* sdFound = (SoundTable::Sound*)tblSounds[iClass].tSounds.Find(&sdForSearch);

					if (!(sdCur = FindModifiedSound(soundBase, feature)))
					{
						Sound* soundWithMod = sdCur = new (pSounds.New()) Sound(chrWithMod, feature);
						tblSounds[iClass].ConfirmSound(soundWithMod, FT_MODIFIER);//со вставкой столбца
					}
				}
				else if (!SoundExists(chr))
				{
					tblSounds[iClass].ConfirmSound(soundBase);
				}
			}
		}
		return sdCur;
	}

	void EndSubmitWordForms()
	{
		for (int i = 0; i < FT_NSOUNDCLASSES; i++)
		{
			tblSounds[i].CountRows();
		}
	}
	SoundTable::Iterator* Iterator(int iClass)
	{
		return new SoundTable::Iterator(&tblSounds[iClass].tSounds);
	}
	//private:
	void CopyFeatures(int* fTo, int* fFrom)
	{
		for (int iFType = 0; iFType < FT_NFEATURETYPES; iFType++)
		{
			if (!fFrom)
				fTo[iFType] = 1;
			else
				fTo[iFType] = fFrom[iFType];
		}
	}
	LPTSTR GetFeatureNames(int* ftrs, int iFType, LPTSTR buf)
	{
		buf[0] = '\0';

		Feature* f;
		for (BTree::Walker w(&tFeatures); f = (Feature*)w.Next();)
		{
			if (f->iType == iFType && (f->iClass == ftrs[FT_CLASS] || f->iClass == FT_COARTICULATION)) //FT_COARTICULATION — общее для С и Г?
			{
				if (f->value & ftrs[iFType])
				{
					lstrcat(buf, f->abbr);
					lstrcat(buf, L" ");
				}
			}
		}

		return buf;
	}

	bool SoundExists(int iChr)
	{
		if (!tblSounds[FT_CONSONANT].IsSoundCodeOK(iChr)) //если согласных нет, то сломается, поэтому надо делать класс IPAAll
			return false;
		else if (ipaAll[iChr])
			return ipaAll[iChr]->exists;

		return false;
	}
	bool SoundIsInIPA(int iChr)
	{
		if (!tblSounds[FT_CONSONANT].IsSoundCodeOK(iChr)) //если согласных нет, то сломается, поэтому надо делать класс IPAAll
			return false;
		else
			return !!ipaAll[iChr];
	}

	bool BuildPotentialSoundTable(int iClass, LPTSTR txtTable, int iHorFType, bool doConfirmAllSounds)
	{
		SoundTable* table = &tblSounds[iClass];
		if (!table->InitIPAMemory(&ipaAll))
			return false;

		Parser parser(txtTable, L"\t\r\n", PARSER_SKIPNEWLINE);
		LPTSTR word;

		CopyFeatures(curFeatureValue, NULL);
		Feature* feature, *ftLast = NULL;

		table->UnsetRow(FT_CLASS);
		table->NextRow(FT_CLASS);
		table->AddRowValue(FT_CLASS, iClass, FT_SET);

		table->UnsetRow(iHorFType);
		table->UnsetRow(FT_MANNER);

		int iFtType = FT_NONE;

		while (word = parser.Next())
		{
			if (parser.IsItemFirstInLine())
			{
				table->UnsetRow(iHorFType);

				ftLast = NULL;

				if (parser.IsItemEmpty())
				{
					switch (iFtType)
					{
					case FT_NONE:
						//curFeatureValue = 1;
						break;
					case FT_PLACE:
					case FT_COARTICULATION:
						iFtType = FT_NONE;
						break;
					}
				}
				else
				{
					table->NextRow(FT_MANNER);

					switch (iFtType)
					{
					case FT_NONE:
						iFtType = FT_MANNER;
						break;
					case FT_PLACE:
					case FT_COARTICULATION:
						iFtType = FT_MANNER;
						//curFeatureValue[iFtType] = modFeatureValue[iFtType];
						break;
					case FT_SOUND:
						iFtType = FT_MANNER;
					}
				}
			}
			else
			{
				switch (iFtType)
				{
				case FT_NONE:
					iFtType = iHorFType;
					break;
				case FT_MANNER:
					iFtType = FT_SOUND;
					break;
				}
				table->NextRow(iHorFType);
			}

			switch (iFtType)
			{
			case FT_PLACE:
			case FT_MANNER:
			case FT_COARTICULATION:
				if (parser.IsItemEmpty())
				{
					if (ftLast) table->AddRowValue(iFtType, ftLast->value, FT_SET);
				}
				else
				{
					if (ftLast) table->AddRowValue(iFtType, ftLast->value, FT_UNSET);

					if (word[0] != '-')
					{
						Feature ftForSearch(word, iClass, iFtType);
						if (!(feature = (Feature*)tFeatures.Find(&ftForSearch)))//ужас: (Feature*), поэтому надо шаблонно, но тоже тяжко
						{
							feature = new (pFeatures.New()) Feature(ftForSearch.abbr, iClass, iFtType, curFeatureValue[iFtType]);
							curFeatureValue[iFtType] = curFeatureValue[iFtType] * 2;
							//curFeatureValue *= 2;//НЕ ПАШЕТ
							tFeatures.Add(feature);
						}
						table->AddRowValue(iFtType, feature->value, FT_SET);

						ftLast = feature;
					}
				}
				break;
			case FT_SOUND:
				if (!parser.IsItemEmpty())
				{
					table->PutSoundInIPATable(word[0], doConfirmAllSounds);
				}
				//ругаться, если есть!
				break;
			}
		}
		return true;
	}
};
#include "segmentizer.h"
#include "ipaquery.h"
#include "replacer.h"