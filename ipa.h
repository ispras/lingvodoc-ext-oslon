#include "parser.h"
#include "pool.h"
#include "btree.h"
#include "datastructures.h"

#define  COUNT(arr) sizeof(arr)/sizeof(arr[0])

LPTSTR _tblPostModifiers =
  L"	губ	пал	прд	зпд	эгл	гло	эгд	зуб	крт	алв	тон	пдг	суж	нос	длг	огб	\r\n"
   "-	ʷ	ʲ	ʰ	ʱ	ˤ	ˀ	ˁ	̪	̆	̠	ʼ	ˑ	̈	̃	ː	̊	";
   
 
LPTSTR _tblVowels =
  L"	-	ОГУ	-	ОГУ	-	ОГУ	\r\n"
   "	ПЕР		ЦНТ		ЗАД		\r\n"
   "В	i	y	ɨ	ʉ	ɯ	u	\r\n"
   "ВН	ɪ	ʏ				ʊ	\r\n"
   "ВС	e	ø	ɘ	ɵ	ɤ	o	\r\n"
   "С			ə				\r\n"
   "СН	ɛ	œ	ɜ	ɞ	ʌ	ɔ	\r\n"
   "НН	æ		ɐ				\r\n"
   "Н	a	ɶ			ɑ	ɒ	";


LPTSTR _tblConsonants =
  L"	-	З	-	З	-	З	-	З	-	З	-	З	-	З	-	З	-	З	-	З	-	З	-	З	\r\n"
   "	ГГ		ГЗ		ЗЗ		АЛ		ПА		РТф		ПАл		ВЕЛ		УВ		ФАР		ЭГЛ		ГЛО		\r\n"
   "	ГУБ				ЗУБ						РТФ		ПАЛ		ЗЯЗ				ЛАР						\r\n"
 "НОС		m		ɱ				n				ɳ		ɲ		ŋ		ɴ							\r\n"
 "ВЗР	p	b					t	d			ʈ	ɖ	c	ɟ	k	ɡ	q	ɢ			ʡ		ʔ		\r\n"	
 "СВИ							s	z	ʃ	ʒ	ʂ	ʐ	ɕ	ʑ											\r\n"
 "ФРИ	ɸ	β	f	v	θ	ð							ç	ʝ	x	ɣ	χ	ʁ	ħ	ʕ			h	ɦ	\r\n"
 "АПП	ʍ	w		ʋ				ɹ				ɻ		j		ɰ									\r\n"
 "1УД				ⱱ			ɾ					ɽ													\r\n"
 "ДРО		ʙ						r										ʀ	ʜ			ʢ			\r\n"
 "БФР							ɬ	ɮ																	\r\n"	
 "БАП								l				ɭ		ʎ		ʟ									\r\n"
 "Б1У							ɺ																		";


//а куда ɥ?

LPTSTR _tblReplacements = 
L":ː\r\n"
 "gɡ";
 

enum
{
	FT_CONSONANT,
	FT_VOWEL,
	FT_MODIFIER,
	FT_UNKNOWNSOUND,
	FT_NSOUNDCLASSES,	
};

enum
{
	FT_CLASS,
	FT_COARTICULATION,
	FT_PLACE,
	FT_MANNER,
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
		
		
	struct Sound : public BNode
	{
	public:
		bool	canExist;
		bool	exists;
		//bool	isPreModifier;
		//bool	isPostModifier;
		//int		features;
		int		iRow;
		int		iCol;
		int		feature[FT_NFEATURETYPES];
		Row*	row[FT_NFEATURETYPES];
		TCHAR	Symbol[8];
		TCHAR	symbolToReplaceBy;
		void*	dataExtra;
		Sound*	nextModified;

		Sound(TCHAR* _symbol, int* _feature = NULL)
		{
			if (_symbol)
			{
				lstrcpy(Symbol, _symbol);
				canExist = true;
			}
			else
				canExist = false;
			
			symbolToReplaceBy = 0;
			exists = false;
			dataExtra = nextModified = NULL;
			
			if (_feature)
			{
				for (int iFType = 0; iFType < FT_NFEATURETYPES; iFType++)
				{
					row[iFType] = NULL;
					feature[iFType] = _feature[iFType];
				}
			}
		}

		Sound(int _code, Row** rows = NULL)//, bool _isPreModifier = false, bool _isPostModifier = false)
		{
			//code = _code;
			//features = _features;
			Symbol[0] =_code;
			Symbol[1] = '\0';
			symbolToReplaceBy = 0;
			
			dataExtra = nextModified = NULL;
			canExist = true;
			exists = false;
			//isPreModifier = _isPreModifier;
			//isPostModifier = _isPostModifier;
			
			//features = 0;
			
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
			Sound* nd1 = (Sound*)_nd1;//поэтому надо шаблонно!!
			Sound* nd2 = (Sound*)_nd2;
/*
			if (nd1->features > nd2->features)
				return 1;
			if (nd1->features < nd2->features)
				return -1;
			return 0;
*/


			for (int iFType = FT_NFEATURETYPES - 1; iFType >= 0; iFType--)
			{
				if (nd1->feature[iFType] > nd2->feature[iFType])
					return 1;
				if (nd1->feature[iFType] < nd2->feature[iFType])
					return -1;
			}
			return 0;
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
			memset(ipaAll, sz, 0);
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
		
		switch (flag & (FT_SET|FT_UNSET))
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

class IPA
{
//	friend Segmentizer;
public://временно вм. friend
	Pool<SoundTable::Sound>	pSounds;
	Pool<Feature>	pFeatures;
	SoundTable::Sound*ipaAll;
	FeatureTree		tFeatures;
	int 			curFeatureValue[FT_NSOUNDCLASSES];
//	int 			modFeatureValue[FT_NSOUNDCLASSES];
public://временно
	SoundTable		tblSounds[FT_NSOUNDCLASSES];
	Pool<TCHAR>		pString;
public:
	LPTSTR tblPostModifiers, tblConsonants, tblVowels, tblReplacements;

	IPA() : pString(3000), pSounds(100)
	{
		ipaAll = NULL;

		//копируем пока просто из глобальных литералов, потом как-то иначе будет
		tblPostModifiers = 	pString.New(_tblPostModifiers, wcslen(_tblPostModifiers)+1);
		tblConsonants = 	pString.New(_tblConsonants, wcslen(_tblConsonants)+1);
		tblVowels = 		pString.New(_tblVowels, wcslen(_tblVowels)+1);
		tblReplacements = 	pString.New(_tblReplacements, wcslen(_tblReplacements)+1);
		
		BuildPotentialSoundTable(FT_MODIFIER, tblPostModifiers, FT_COARTICULATION);

		BuildPotentialSoundTable(FT_CONSONANT, tblConsonants, FT_PLACE);
		BuildPotentialSoundTable(FT_VOWEL, tblVowels, FT_PLACE);
		BuildReplacementTable(tblReplacements);
		
		tblSounds[FT_UNKNOWNSOUND].InitIPAMemory(&ipaAll);//плохо, что надо явно вызывать!!!		
	}
	~IPA()
	{
		free(ipaAll);
	}
	//void AddUnknownSound()
	//{
	//}
	SoundTable::Sound* FindModifiedSound(SoundTable::Sound* sndBase, int* feature)
	{
		for (SoundTable::Sound* soundMod = sndBase->nextModified; soundMod; soundMod = soundMod->nextModified)
		{
			SoundTable::Sound sdForCompare(NULL, feature);
			if (!tblSounds[sndBase->feature[FT_CLASS]].tSounds.CompareNodes(soundMod, &sdForCompare, NULL))
				return soundMod;
		}
		return NULL;
	}
	int GetPostModifiers(LPTSTR pInWord, LPTSTR chrWithMod, int* feature)
	{
		SoundTable::Sound* soundBase = &ipaAll[*pInWord];
		int nPostModifiers = 0;
		
		TCHAR chrMod;
		while (chrMod = pInWord[1])
		{
			if (ipaAll[chrMod].feature[FT_CLASS] == FT_MODIFIER)
			{
				SoundTable::Sound* soundMod = &ipaAll[chrMod];
				if (nPostModifiers == 0)//т.е. это первый
				{
					chrWithMod[0] = soundBase->Symbol[0];
					CopyFeatures(feature, soundBase->feature);
				}
				
				//if (nPostModifiers > ...)//чтоб не было больше 5, например
				
				nPostModifiers++;
				
				feature[FT_COARTICULATION] |= soundMod->feature[FT_COARTICULATION];
				
				chrWithMod[nPostModifiers] = chrMod;
			}
			else break;
			pInWord++;
		}

		chrWithMod[nPostModifiers + 1] = '\0';

		return nPostModifiers;
	}
	SoundTable::Sound* GetSoundWithReplacement(TCHAR* pChr)
	{
		SoundTable::Sound* sound = &ipaAll[*pChr];
		if (sound->symbolToReplaceBy)
		{
			*pChr = sound->symbolToReplaceBy;
			sound = &ipaAll[*pChr];
		}
		return sound;
	}

	bool SubmitWordForm(LPTSTR word)//будет меняться подстановками!
	{
		for (LPTSTR pInWord = word; *pInWord; pInWord++)
		{
			SoundTable::Sound* soundBase = GetSoundWithReplacement(pInWord);
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
						SoundTable::Sound* soundWithMod = new (pSounds.New()) SoundTable::Sound(chrWithMod, feature);//ПОТОМ НЕ УДАЛЯЕМ! ПОЗАБОТИТЬСЯ ОБ ЭТОМ!
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
//out(tblSounds[FT_CONSONANT].llRows[FT_PLACE].first->isEmpty);
		for (int i = 0; i < FT_NSOUNDCLASSES; i++)
		{
			tblSounds[i].CountRows();
		}
	}
	SoundTable::Iterator* Iterator(int iClass)
	{
		//return new...
BTree*_t=&tblSounds[iClass].tSounds;
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
	void BuildReplacementTable(LPTSTR tblReplacements)
	{
		Parser parser(tblReplacements, L"\t\r\n", PARSER_SKIPNEWLINE);
		
		LPTSTR word;
		while (word = parser.Next())
		{
			ipaAll[word[0]].symbolToReplaceBy = word[1];
		}
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
							curFeatureValue[iFtType] = curFeatureValue[iFtType]* 2;
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


class Segmentizer
{
public:
	LPTSTR				pInWord;
	LPTSTR				pOldInWord;
	LPTSTR				text;
	SoundTable::Sound*	sndCurrent;
	SoundTable::Sound*	sndPrevious;
public:
	IPA*				ipa;//на самом деле friend Query?

	Segmentizer(IPA* _ipa, LPTSTR _text)
	{
		ipa = _ipa;
		pInWord = pOldInWord = NULL;
		text = _text;
		sndCurrent = NULL;
		sndPrevious = NULL;
	}
	SoundTable::Sound* Current()
	{
		return sndCurrent; 
	}
	SoundTable::Sound* PeekPrevious()
	{
		return sndPrevious; 
	}
	SoundTable::Sound* PeekNext()
	{
		return GetNext(true); 
	}
	SoundTable::Sound* GetNext(bool doPeek = false)
	{
		SoundTable::Sound* soundBase, *sound;
		LPTSTR pos = pInWord;

		if (!pos)
			pos = text;
		if (!*pos)
			return NULL;

		if (!doPeek)
			pOldInWord = pos;
		
		TCHAR chr = *pos;
		soundBase = &ipa->ipaAll[chr];
			
		int	feature[FT_NFEATURETYPES];
		TCHAR chrWithMod[9];
		int nPostModifiers = ipa->GetPostModifiers(pos, chrWithMod, feature);
		pos += nPostModifiers;
			
		if (!nPostModifiers)
			sound = soundBase;
		else
			sound = ipa->FindModifiedSound(soundBase, feature);
		
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
