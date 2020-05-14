#define QF_NOTHING					0x0
#define QF_FEATURE					0x2
#define QF_SOUND					0x4
#define QF_FRAGMENT					0x8
#define QF_BEGINNING				0x20
#define QF_ITERATE					0x40
#define QF_OBJECTONLYONCE			0x100
#define QF_CONTEXTONLYONCE			0x2000
#define QF_OPTIONAL					0x4000
#define QF_DELETENULLPREV			0x8000

#define ST_ERROR				-1
#define ST_EMPTY				0
#define ST_EMPTYAUTOFILL		10
#define ST_SOUND				11
#define ST_FRAGMENT				12
#define ST_FRAGMENTMAYBE		13
#define ST_NULL					16

#define ST_EQUAL				100
#define ST_BOTHNOTFOUND			101
#define ST_ONEEMPTY				102
#define ST_UNEQUAL				120

#define MAX_SZCONDTIONTEXT		20

class Condition;//надо без этого!
class Condition : public LinkedElement<Condition>, public OwnNew
{
public:
	LPTSTR		title;
	void*		dataExtra;
	int			intExtra;
	int			flags;
	int			nIterate;
	int			iSyllableToLook;
	int			iCurSyllable;

	class Segment
	{
	public:
		int 		flag;
		int			feature[FT_NFEATURETYPES];
		TCHAR		txtCondition[MAX_SZCONDTIONTEXT];
		//LPTSTR		txtCondition;
		short		wasAlready;
		Sound*		sound;

		Segment(LPTSTR _txt)
		{
			if (_txt && !_txt[0]) _txt = NULL;

			if (!_txt)
				txtCondition[0] = L'\0';
			else
			{
				wcsncpy(txtCondition, _txt, MAX_SZCONDTIONTEXT);
				txtCondition[MAX_SZCONDTIONTEXT] = L'\0';
			}

			wasAlready = false;
			flag = QF_NOTHING;

			//обнулять как-то надо, тут нужен к-р особый для «совокупности признаков»?
			for (int iFType = 0; iFType < FT_NFEATURETYPES; iFType++)
				feature[iFType] = 0;
		}
		bool Init(IPA* ipa)
		{
			if (!txtCondition[0]) return false;
			flag = 0;
			LPTSTR txtFeature;

			switch (txtCondition[0])
			{
			case L'#':
				flag |= QF_BEGINNING;//в конце иначе	
				break;
			case L'Г'://это временно, потом будут разные группы звуков, прямо внутри форм можно будет
			case L'С':
				txtFeature = txtCondition;
				goto FindFeature;
			case L'(':
				flag |= QF_OPTIONAL;
			case L'-'://это пока не действует
			case L'+':
				txtFeature = txtCondition + 1;
			FindFeature:
				{
					Feature ftForSearch(txtFeature);
					Feature* ftFound = (Feature*)ipa->tFeatures.Find(&ftForSearch);//будет искать каждый раз, если задано несущ.
					if (ftFound)
					{
						feature[ftFound->iType] = ftFound->value;
						feature[FT_CLASS] = ftFound->iClass;
						flag |= QF_FEATURE;
					}
				}
				break;
			default:
				if (sound = ipa->GetSound(txtCondition[0]))
					flag |= QF_SOUND;
			}
			return true;
		}
		int Check(Sound* sdThis, int iFType = FT_NFEATURETYPES - 1) //последнее непонятно
		{
			if (sdThis)
			{
				if (flag & QF_FEATURE)
				{
					if (!CompareFeaturesOr(sdThis->feature, feature, iFType))//т.е. текущая годится как контекст в принципе, но только по классу, т.е. С или Г!!
					{
						wasAlready++;
						return ST_SOUND;
					}
				}
				else if (flag & QF_SOUND)
				{
					if (sdThis == sound)
						return ST_SOUND;
				}
			}
			if (flag & QF_OPTIONAL)
				return ST_NULL;
			else
				return ST_EMPTY;
		}
	};

	Segment		sgPrev;
	Segment		sgThis;
	Segment		sgNext;

	Condition(LPTSTR _ftThis, LPTSTR _ftPrev, LPTSTR _ftNext, int _flags = 0, LPTSTR _title = NULL, int _extraInt = 0, int _nIterate = 2, int _iSyllable = -1) : sgThis(_ftThis), sgPrev(_ftPrev), sgNext(_ftNext)
	{
		title = _title;
		flags = _flags;
		intExtra = _extraInt;
		nIterate = _nIterate;
		iSyllableToLook = _iSyllable;
		//		condition = _condition;
		//		iClass = _iClass;
		//		feature = NULL;
		//		Reset();

		//		sgThis.Init(ipa);
		//		sgPrev.Init(ipa);
		//		sgNext.Init(ipa);
	}
	LPTSTR AutoTitle(LPTSTR buf, int szPad = 0)
	{
		buf[0] = L'\0';
		if (sgPrev.txtCondition[0])
			wcscat(buf, sgPrev.txtCondition);
		wcscat(buf, L"_");
		if (sgNext.txtCondition[0])
			wcscat(buf, sgNext.txtCondition);

		if (szPad)
		{
			int i;
			for (i = wcslen(buf); i < szPad; i++)
				buf[i] = L'_';
			buf[i] = L'\0';
		}

		return buf;
	}
	void Reset()
	{
		sgPrev.wasAlready = 0;
		sgThis.wasAlready = 0;
		sgNext.wasAlready = 0;
		iCurSyllable = 0;
		//wasObject = wasContext = wasObjectInContext = false;//sgThis.feature = NULL;
		//sgPrev.feature = NULL;
		//sgNext.feature = NULL;
	}
	bool CheckThisFeature(int iFType, int val, IPA* ipa)
	{
		if (sgThis.flag == QF_NOTHING) sgThis.Init(ipa);
		return sgThis.feature[iFType] & val;
	}

	int Check(Segmentizer* sgmtzr, bool *wasLeft = NULL)
	{
		if (sgThis.flag == QF_NOTHING) sgThis.Init(sgmtzr->ipa);
		if (sgPrev.flag == QF_NOTHING) sgPrev.Init(sgmtzr->ipa);
		if (sgNext.flag == QF_NOTHING) sgNext.Init(sgmtzr->ipa);

		Sound* sdThis = sgmtzr->Current(),
			*sdAdjacent;

		if (iSyllableToLook != -1)
		{
			if (sdThis->feature[FT_CLASS] & FT_VOWEL)
			{
				sdAdjacent = sgmtzr->PeekPrevious();
				if (!sdAdjacent)
					iCurSyllable++;
				else if (!(sdAdjacent->feature[FT_CLASS] & FT_VOWEL))
					iCurSyllable++;
			}
			if (iCurSyllable != iSyllableToLook)
				return ST_EMPTY;
		}

		if ((flags & QF_OBJECTONLYONCE) && (sgThis.wasAlready))
			return ST_EMPTY;

		if ((flags & QF_CONTEXTONLYONCE) && ((sgPrev.wasAlready) || (sgNext.wasAlready)))
			return ST_EMPTY;
		//		if ((flags & QF_OBJECTINCONTEXTONLYONCE) && wasObjectInContext)
		//			return false;

				//проставляет wasAlready??
		sgPrev.Check(sdThis, FT_CLASS);
		sgNext.Check(sdThis, FT_CLASS);

		int ret = sgThis.Check(sdThis, FT_CLASS);

		if (ret == ST_EMPTY) return ST_EMPTY;

		if (sgPrev.flag & QF_BEGINNING)
		{
			if (!sgmtzr->IsFirst()) return ST_EMPTY;
		}
		else if (sgPrev.flag != QF_NOTHING)
		{
			switch (sgPrev.Check(sgmtzr->PeekPrevious()))
			{
			case ST_EMPTY:
				return ST_EMPTY;
			case ST_SOUND:
				if (wasLeft) *wasLeft = true;
			}
		}

		if (sgNext.flag != QF_NOTHING)
			if (sgNext.Check(sgmtzr->PeekNext()) == ST_EMPTY) return ST_EMPTY;

		return ret;
	}

	int GetFirstMatchingFragment(IPA* ipa, Sound** sound, LPTSTR wIn, LPTSTR wOut, bool* wasPrev = NULL)
	{
		Segmentizer sgmntzr(ipa, wIn);
		Reset();

		if (!wIn)
		{
			if (sound) *sound = NULL;
			return ST_EMPTY;
		}

		int ret = ST_ERROR;
		while (sgmntzr.GetNext())
		{
			ret = Check(&sgmntzr, wasPrev);
			if (ret == ST_SOUND || ret == ST_NULL)
				break;
		}

		switch (ret)
		{
		case ST_ERROR:
		case ST_EMPTY:
			return ST_ERROR;
		case ST_NULL:
			if (sound) *sound = NULL;
			if (wOut) 	wcscpy(wOut, L"");
			break;
		case ST_SOUND:
			if (sound) *sound = sgmntzr.Current();
			if (wOut) 	wcscpy(wOut, (*sound)->Symbol);
		}

		if (ret == ST_SOUND && wOut && flags & QF_ITERATE)
		{
			Condition c(sgThis.txtCondition, NULL, NULL);//вот так неловко копируем условие
			Sound* sPrev = sgmntzr.PeekPrevious(); //это пока так, ибо нет движения назад
			Sound* sNext;

			int i = 2;

			while ((sNext = sgmntzr.GetNext()) && i <= nIterate)
			{
				if (c.Check(&sgmntzr) == ST_SOUND)
				{
					ret = ST_FRAGMENT;
					wcscat(wOut, sNext->Symbol);
				}
				else if (sgThis.feature[FT_CLASS] == FT_VOWEL)//проверяем потенциальные д-ги [надо это как-то обобщить]
				{
					Sound* sBase = ipa->GetBaseSound(sNext);

					if (!sBase) break;

					if (wcschr(_listExtraGlides, sBase->Symbol[0])) //слева направо
					{
						ret = ST_FRAGMENTMAYBE;
						wcscat(wOut, sNext->Symbol);
					}
					else if (sPrev)
					{
						sBase = ipa->GetBaseSound(sPrev);
						LPTSTR posFound = wcschr(_listExtraGlides, sBase->Symbol[0]);
						if (posFound) //слева направо
						{
							ret = ST_FRAGMENTMAYBE;
							TCHAR buf[20];
							wcscpy(buf, wOut);

							//int iSymbol = _listExtraGlides - posFound;

							wcscpy(wOut, sPrev->Symbol);
							//тут можно было бы заменять: wcscpy(wOut, _listExtraGlidesVowels[);

							wcscat(wOut, buf);
						}
					}
				}
				else break; //но надо откатить назад?
				i++;
			}
		}

		return ret;
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
	Condition* AddCondition(LPTSTR _ftThis, LPTSTR _ftPrev, LPTSTR _ftNext, int flags = 0, LPTSTR _title = NULL, int _extraInt = 0, int _iSyllable = -1, int _iIterate = 2)
	{
		Condition* c = ::new Condition(_ftThis, _ftPrev, _ftNext, flags, _title, _extraInt, _iIterate, _iSyllable);

		llConditions.Add(c);

		return c;
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
	int CheckCurrentCondition()//вроде лишнее
	{
		if (!cndCur)
			return false;

		return cndCur->Check(sgmtzr);
	}
};