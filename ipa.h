#pragma once

#include "stringfunctions.h"
#include "parser.h"
#include "pool.h"
#include "btree.h"
#include "datastructures.h"

#include "ipatables.h"
#include "ipareplacements.h"

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
	FT_CLASS,
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

int CompareFeaturesNumberOfDifferences(int* f1, int* f2)
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
	class Row : public LinkedElement<Row>
	{
	public:
		bool		isEmpty;
		int			valBase;
		int			valMod;
		int			i;
		Row()
		{
			i = 0;
			isEmpty = true;
			valBase = valMod = 0;
		}
	};

	//сделать SoundRef

	class Sound : public BNode
	{
	public:
		bool			canExist;
		bool			exists;
		TCHAR			Symbol[8];
		void*			dataExtra;
		Sound*			nextModified;
		Row*			row[FT_NFEATURETYPES];
		int				feature[FT_NFEATURETYPES];

		Sound(TCHAR* _symbol, int* _feature = NULL)
		{
			if (_symbol)
			{
				lstrcpy(Symbol, _symbol);
				canExist = true;
			}
			else
				canExist = false;

			//symbolToReplaceBy[0] = '\0';
			exists = false;
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
			Symbol[1] = '\0';

			dataExtra = nextModified = NULL;
			canExist = true;
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
		int RowNumber(int iFType)
		{
			if (!row[iFType])
				return -1;
			return row[iFType]->i;
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

	Sound*			ipaAll;
	int				nipaAll;

	LinkedList<Row> llRows[FT_NFEATURETYPES];
	Row*			rowCurrent[FT_NFEATURETYPES];

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

	SoundTable()
	{
		ipaAll = NULL;
		for (int iFType = 0; iFType < FT_NFEATURETYPES; iFType++)
			rowCurrent[iFType] = NULL;
	}
	~SoundTable()
	{
		for (int iFType = 0; iFType < FT_NFEATURETYPES; iFType++)
		{
			llRows[iFType].DestroyAll();
		}
	}

	void InitIPAMemory(Sound** _pipaAll)
	{
		nipaAll = 0xffff;
		if (!*_pipaAll)
		{
			int sz = sizeof(Sound)*nipaAll;
			*_pipaAll = (Sound*)malloc(sz);//не new, чтоб избежать к-ра в цикле
			memset(*_pipaAll, 0, sz);
		}
		ipaAll = *_pipaAll;
	}

	void AddUnknownSound(int iChr)
	{
		Sound* sound = new (&ipaAll[iChr]) Sound(iChr);
		sound->feature[FT_CLASS] = FT_UNKNOWNSOUND;
		sound->feature[FT_MANNER] = iChr;
		sound->exists = true;
		sound->canExist = false;

		tSounds.Add(sound);
	}

	void ConfirmSound(Sound* sound, int iClass = -1)
	{
		sound->exists = true;
		tSounds.Add(sound);

		if (iClass == FT_MODIFIER)
		{
			Sound* sdMain = ipaAll + sound->Symbol[0];
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
				row = new Row;
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
		Row* row;
		if (!llRows[iFType].first)
		{
			row = new Row;
			llRows[iFType].Add(row);
		}
		else if (!rowCurrent[iFType])
			row = llRows[iFType].first;
		else if (!rowCurrent[iFType]->next) //то же самое действие
		{
			row = new Row;
			llRows[iFType].Add(row);
		}
		else
		{
			row = rowCurrent[iFType]->next;
		}
		rowCurrent[iFType] = row;
	}
	bool PutSoundInIPATable(int iChr)
	{
		if (!IsSoundCodeOK(iChr))
			return false;
		//else if (ipaAll[iChr].canExist)
		//	return false;

		//Sound* sound = 
		new (&ipaAll[iChr]) Sound(iChr, rowCurrent);//, iRow, iCol, _isPreModifier, _isPostModifier);
		//	tSounds.Add(sound);
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
					llRows[iFType].Delete(row, true);
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
	Sound*			ipaAll;
	Pool<Sound>		pSounds;
	Pool<Feature>	pFeatures;
	FeatureTree		tFeatures;
	int 			curFeatureValue[FT_NSOUNDCLASSES];
	SoundTable		tblSounds[FT_NSOUNDCLASSES];
	Pool<TCHAR>		pString;

public:
	LPTSTR tblPostModifiers, tblConsonants, tblVowels;//, tblReplaceLat, tblReplaceCyr;

	IPA() : pString(3000), pSounds(100)
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

		BuildPotentialSoundTable(FT_MODIFIER, tblPostModifiers, FT_COARTICULATION);

		BuildPotentialSoundTable(FT_CONSONANT, tblConsonants, FT_PLACE);
		BuildPotentialSoundTable(FT_VOWEL, tblVowels, FT_PLACE);

		tblSounds[FT_UNKNOWNSOUND].InitIPAMemory(&ipaAll);//плохо, что надо явно вызывать!!!		
	}
	~IPA()
	{
		free(ipaAll);
	}
	//void AddUnknownSound()
	//{
	//}
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
		Sound* soundBase = &ipaAll[*pInWord];
		int nPostModifiers = 0;

		while (true)
		{
			Sound* soundMod = &ipaAll[pInWord[1]];

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

		chrWithMod[nPostModifiers + 1] = '\0';

		return nPostModifiers;
	}

	Sound* GetSound(TCHAR pChr)
	{
		if (pChr == 0)
			return NULL;
		return &ipaAll[pChr];
	}

	Sound* GetBaseSound(Sound* sound)
	{
		return GetSound(sound->Symbol[0]);
	}


	bool SubmitWordForm(LPTSTR word)//будет меняться подстановками!
	{
		for (LPTSTR pInWord = word; *pInWord; pInWord++)
		{
			Sound* soundBase = &ipaAll[*pInWord];//GetSoundWithReplacement(pInWord);
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

					if (!FindModifiedSound(soundBase, feature))
					{
						SoundTable::Sound* soundWithMod = new (pSounds.New()) SoundTable::Sound(chrWithMod, feature);
						tblSounds[iClass].ConfirmSound(soundWithMod, FT_MODIFIER);//со вставкой столбца
					}
				}
				else if (!SoundExists(chr))
				{
					tblSounds[iClass].ConfirmSound(soundBase);
				}
			}
		}
	}
	bool EndSubmitWordForms()
	{
		for (int i = 0; i < FT_NSOUNDCLASSES; i++)
		{
			tblSounds[i].CountRows();
		}
	}
	SoundTable::Iterator* Iterator(int iClass)
	{
		//return new...
		BTree*_t = &tblSounds[iClass].tSounds;
		SoundTable::Iterator* it = new SoundTable::Iterator(_t);
		return it;
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
	bool SoundExists(int iChr)
	{
		if (!tblSounds[FT_CONSONANT].IsSoundCodeOK(iChr)) //если согласных нет, то сломается, поэтому надо делать класс IPAAll
			return false;
		else if (ipaAll[iChr].exists)
			return true;
		return false;
	}
	bool SoundIsInIPA(int iChr)
	{
		if (!tblSounds[FT_CONSONANT].IsSoundCodeOK(iChr)) //если согласных нет, то сломается, поэтому надо делать класс IPAAll
			return false;
		else if (ipaAll[iChr].canExist)
			return true;
		return false;
	}

	void BuildPotentialSoundTable(int iClass, LPTSTR txtTable, int iHorFType)
	{
		SoundTable* table = &tblSounds[iClass];
		table->InitIPAMemory(&ipaAll);

		Parser parser(txtTable, L"\t\r\n", PARSER_SKIPNEWLINE);
		LPTSTR word;

		CopyFeatures(curFeatureValue, NULL);
		Feature* feature, *ftLast;

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
					table->PutSoundInIPATable(word[0]);
				}
				//ругаться, если есть!
				break;
			}
		}
	}
};


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
		sound = soundBase = &ipa->ipaAll[chr];

		if (doSearchModified)
		{
			int	feature[FT_NFEATURETYPES];
			TCHAR chrWithMod[9];
			int nPostModifiers = ipa->GetPostModifiers(pos, chrWithMod, feature);
			pos += nPostModifiers;

			if (nPostModifiers)
				sound = ipa->FindModifiedSound(soundBase, feature);
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
};
/////////////////////////////////////////////////////////////////////////////
#define QF_NOTHING					0x0
#define QF_SOMETHING				0x2
#define QF_BEGINNING				0x20
#define QF_OBJECTONLYONCE			0x400
#define QF_CONTEXTONLYONCE			0x2000

class Condition;//надо без этого!
class Condition : public LinkedElement<Condition>, public OwnNew
{
public:
	LPTSTR		title;

	class Segment
	{
	public:
		int 		flag;
		int			feature[FT_NFEATURETYPES];
		LPTSTR		txtFeature;
		short		wasAlready;

		Segment(LPTSTR _txt)
		{
			if (_txt) if (!_txt[0]) _txt = NULL; //у нас && неправильный

			txtFeature = _txt;
			wasAlready = false;
			flag = QF_NOTHING;

			if (txtFeature)
			{
				if (!lstrcmp(txtFeature, L"#"))
					flag = QF_BEGINNING;//в конце иначе	
			}
			//обнулять как-то надо, тут нужен к-р особый для «совокупности признаков»?
			for (int iFType = 0; iFType < FT_NFEATURETYPES; iFType++)
				feature[iFType] = 0;
		}
		bool Init(IPA* ipa)
		{
			if (!txtFeature) return false;

			Feature ftForSearch(txtFeature/*пока одно*/);
			Feature* ftFound = (Feature*)ipa->tFeatures.Find(&ftForSearch);//будет искать, каждый раз, если задано несущ.
			if (ftFound)
			{
				feature[ftFound->iType] = ftFound->value;
				feature[FT_CLASS] = ftFound->iClass;
				flag = QF_SOMETHING;
			}
			return true;
		}
	};

	Segment		sgPrev;
	Segment		sgThis;
	Segment		sgNext;
	int			flags;

	Condition(LPTSTR _ftThis, LPTSTR _ftPrev, LPTSTR _ftNext, int _flags = 0, LPTSTR _title = NULL) : sgThis(_ftThis), sgPrev(_ftPrev), sgNext(_ftNext)
	{
		title = _title;
		flags = _flags;
		//		condition = _condition;
		//		iClass = _iClass;
		//		feature = NULL;
		//		Reset();

		//		sgThis.Init(ipa);
		//		sgPrev.Init(ipa);
		//		sgNext.Init(ipa);
	}
	void Reset()
	{
		sgPrev.wasAlready = 0;
		sgThis.wasAlready = 0;
		sgNext.wasAlready = 0;
		//wasObject = wasContext = wasObjectInContext = false;//sgThis.feature = NULL;
		//sgPrev.feature = NULL;
		//sgNext.feature = NULL;
	}
	bool CheckThisFeature(int iFType, int val, IPA* ipa)
	{
		if (sgThis.flag == QF_NOTHING) sgThis.Init(ipa);
		return sgThis.feature[iFType] & val;
	}
	bool Check(Segmentizer* sgmtzr)
	{
		if (sgThis.flag == QF_NOTHING) sgThis.Init(sgmtzr->ipa);
		if (sgPrev.flag == QF_NOTHING) sgPrev.Init(sgmtzr->ipa);
		if (sgNext.flag == QF_NOTHING) sgNext.Init(sgmtzr->ipa);

		Sound* sdThis = sgmtzr->Current(),
			*sdAdjacent;
		if ((flags & QF_OBJECTONLYONCE) && (sgThis.wasAlready))
			return false;

		if ((flags & QF_CONTEXTONLYONCE) && ((sgPrev.wasAlready) || (sgNext.wasAlready)))
			return false;
		//		if ((flags & QF_OBJECTINCONTEXTONLYONCE) && wasObjectInContext)
		//			return false;
		if (sgPrev.flag != QF_NOTHING)
		{
			if (!CompareFeaturesOr(sdThis->feature, sgPrev.feature, FT_CLASS))//т.е. текущая годится как контекст в принципе, но тольео по классу, т.с. С или Г!!
				sgPrev.wasAlready++;
		}
		if (sgNext.flag != QF_NOTHING)
		{
			if (!CompareFeaturesOr(sdThis->feature, sgNext.feature, FT_CLASS))//т.е. текущая годится как контекст в принципе
				sgNext.wasAlready++;
		}
		if (sgThis.flag != QF_NOTHING)
		{
			if (!CompareFeaturesOr(sdThis->feature, sgThis.feature, FT_CLASS))//т.е. текущая годится как заявленное «это»
				sgThis.wasAlready++;
			else
				return false;
		}

		if (sgPrev.flag & QF_BEGINNING)
		{
			if (sgmtzr->IsFirst())
				return true;
			else
				return false;
		}

		if (sgPrev.flag != QF_NOTHING)
		{
			sdAdjacent = sgmtzr->PeekPrevious();
			if (!sdAdjacent)
				return false;
			if (!CompareFeaturesOr(sdAdjacent->feature, sgPrev.feature))
				return true;
			else
				return false;
		}
		if (sgNext.flag != QF_NOTHING)
		{
			sdAdjacent = sgmtzr->PeekNext();
			if (!sdAdjacent)
				return false;
			if (!CompareFeaturesOr(sdAdjacent->feature, sgNext.feature))
				return true;
			else
				return false;
		}


		//		bool isMatch = !!(sdAdjacent->feature[feature->iType] & feature->value);

		//		return isMatch;
		return true;
		//return false;
	}
};

class Query
{
public:
	Segmentizer* sgmtzr;
	Condition* cndCur;
	LinkedList<Condition> llConditions;
	//IPA* 		ipa;

	Query(/*IPA* _ipa*/)
	{
		//ipa = _ipa;
		cndCur = NULL;
	}
	~Query()
	{
		llConditions.DestroyAll();
	}
	void SetSegmentizer(Segmentizer* _sgmtzr)
	{
		sgmtzr = _sgmtzr;
		ResetConditions();
	}
	void AddCondition(LPTSTR _ftThis, LPTSTR _ftPrev, LPTSTR _ftNext, int flags = 0, LPTSTR _title = NULL)
	{
		Condition* c = ::new Condition(_ftThis, _ftPrev, _ftNext, flags, _title);

		llConditions.Add(c);
	}
	Condition* FirstCondition()
	{
		return cndCur = llConditions.first;
	}
	void ResetConditions()
	{
		for (Condition* c = llConditions.first; c; c = c->next)
		{
			c->Reset();
		}
		cndCur = NULL;
	}
	Condition* NextCondition()
	{
		if (!cndCur)
			cndCur = llConditions.first;
		else
			cndCur = cndCur->next;
		return cndCur;
	}
	bool CheckCondition()
	{
		if (!cndCur)
			return false;

		return cndCur->Check(sgmtzr);
	}
};

/////////////////////////////////////////////////////////////////////////////
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
			symbolToReplace[0] = '\0';
			symbolToReplaceBy[0] = '\0';
		}
	};
	IPA*				ipa;
	Rule*				rules;
	LPTSTR				lang;
	LPTSTR				textRules;
	Pool<TCHAR>			pString;
	Pool<Rule>			pRules;
	Pool<Condition>		pConditions;

	Replacer() : pString(2000), pConditions(10), pRules(10)
	{
		textRules = NULL;

		int nipaAll = 0xffff;
		int sz = sizeof(Rule)*nipaAll;
		rules = (Rule*)malloc(sz);//не new, чтоб избежать к-ра в цикле
		memset(rules, 0, sz);
	}
	~Replacer()
	{
		free(rules);
	}
	void Set(IPA* _ipa, LPTSTR _lang)
	{
		ipa = _ipa;
		lang = _lang;
	}
	Rule* CreateRule(TCHAR replaceWhat, LPTSTR replaceBy)
	{
		Rule* rule = rules + replaceWhat;

		if (rule->symbolToReplace[0]) //значит, уже есть, новое надо привесить к нему
		{
			Rule* ruleNew = new (pRules.New()) Rule;

			Rule *ruleLast = rule;
			for (Rule* r = rule->nextSame; r; r = r->nextSame)
				ruleLast = r;
			ruleLast->nextSame = ruleNew;

			rule = ruleNew;
		}

		rule->symbolToReplace[0] = replaceWhat;
		rule->symbolToReplace[1] = '\0';//пока только один знак
		StrCpyWMax(rule->symbolToReplaceBy, replaceBy, 8);
		return rule;
	}
	bool AddRules(LPTSTR _textRules)
	{
		textRules = pString.New(_textRules, wcslen(_textRules) + 1);

		Parser parser(textRules, L">,|_\r\n", PARSER_SKIPNEWLINE);

		Rule* rule = NULL;
		LPTSTR word;
		LPTSTR fPrev, fThis, fNext;
		fPrev = fThis = fNext = NULL;
		bool isCond = false;
		while (word = parser.Next())
		{
			switch (parser.Separator())
			{
			case '>':
				break;
			case '|':
				isCond = true;
				rule = CreateRule(word[0], word + 1);
				break;
			case '_':
				fPrev = word;
				break;
			case ',':
			case '\r':
			case '\0'://конец
				if (isCond)
				{
					fNext = word;
					rule->condition = new (pConditions.New()) Condition(fThis, fPrev, fNext, 0, NULL);
				}
				else
					rule = CreateRule(word[0], word + 1);

				rule = NULL;
				fPrev = fThis = fNext = NULL;
				isCond = false;
			}
		}
	}
	bool IsCharInTable(TCHAR chr)
	{
		return rules[chr].symbolToReplace[0] != '\0';
	}
	int Convert(LPTSTR bInBeg, LPTSTR bOutBeg)
	{
		Segmentizer sgmntzr(ipa, bInBeg, false);

		TCHAR buf[200];
		bool isCondition = false;
		//for (; *bIn; bIn++)

		Sound* sdCur;
		int sz;
		LPTSTR bIn, bOut;

		for (int iPass = 1; iPass <= 2; iPass++)
		{
			bIn = bInBeg;
			bOut = bOutBeg;


			while (sdCur = sgmntzr.GetNext())
			{
				TCHAR chr = sgmntzr.Current1Char();
				Replacer::Rule* rule;// = &rules[chr];

				for (rule = &rules[chr]; rule; rule = rule->nextSame)
				{
					if (rule->condition)
					{
						switch (iPass)
						{
						case 1:
							isCondition = true;
							goto JustCopy;
							break;
						case 2:
							if (rule->condition->Check(&sgmntzr))
								goto Replace;
							else if (!rule->nextSame)
								goto JustCopy;
						}
					}
					else break;
					if (iPass == 1) break;
				}

				if (!rule)
					goto JustCopy;

			Replace:
				switch (rule->symbolToReplaceBy[0])
				{
				case '\0':
				JustCopy:
					*bOut = chr;
					bOut++;
					break;
				case '@':
					break;
				default:
					sz = wcslen(rule->symbolToReplaceBy);
					wcscpy(bOut, rule->symbolToReplaceBy);
					bOut += sz;
				}
			}
			*bOut = '\0';
			if (iPass == 1)
			{
				if (!isCondition)
					break;
				else
				{
					wcscpy(buf, bOutBeg);
					bInBeg = buf;
					sgmntzr.Set(bInBeg);
				}
			}
		}
		return bOut - bOutBeg;
	}
};
