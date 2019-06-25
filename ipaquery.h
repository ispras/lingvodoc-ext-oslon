#define QF_NOTHING					0x0
#define QF_FEATURE					0x2
#define QF_SOUND					0x4
#define QF_FRAGMENT					0x8
#define QF_BEGINNING				0x20
#define QF_ITERATE					0x40
#define QF_OBJECTONLYONCE			0x100
#define QF_CONTEXTONLYONCE			0x2000

#define ST_ERROR				-1
#define ST_NONE					0
#define ST_EMPTYAUTOFILL		10
#define ST_SOUND				11
#define ST_FRAGMENT				12
#define ST_FRAGMENTMAYBE		13

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
		LPTSTR		txtCondition;
		short		wasAlready;
		Sound*		sound;

		Segment(LPTSTR _txt)
		{
			if (_txt) if (!_txt[0]) _txt = NULL; //у нас логический && неправильный

			txtCondition = _txt;
			wasAlready = false;
			flag = QF_NOTHING;

			//обнулять как-то надо, тут нужен к-р особый для «совокупности признаков»?
			for (int iFType = 0; iFType < FT_NFEATURETYPES; iFType++)
				feature[iFType] = 0;
		}
		bool Init(IPA* ipa)
		{
			if (!txtCondition) return false;

			LPTSTR txtFeature;

			switch (txtCondition[0])
			{
			case L'#':
				flag = QF_BEGINNING;//в конце иначе	
				break;
			case L'Г'://это временно, потом будут разные группы звуков, прямо внутри форм можно будет
			case L'С':
				txtFeature = txtCondition;
				goto FindFeature;
			case L'-':
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
						flag = QF_FEATURE;
					}
				}
				break;
			default:
				if (sound = ipa->GetSound(txtCondition[0]))
					flag = QF_SOUND;
			}
			return true;
		}
		bool Check(Sound* sdThis, int iFType)
		{
			if (flag & QF_FEATURE)
			{
				if (!CompareFeaturesOr(sdThis->feature, feature, iFType))//т.е. текущая годится как контекст в принципе, но только по классу, т.е. С или Г!!
				{
					wasAlready++;
					return true;
				}
				else
					return false;
			}
			else if (flag & QF_SOUND)
			{
				return (sdThis == sound);
			}
			return true;
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
		if (sgPrev.txtCondition)
			wcscat(buf, sgPrev.txtCondition);
		wcscat(buf, L"_");
		if (sgNext.txtCondition)
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

	bool Check(Segmentizer* sgmtzr)
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
				return false;
		}

		if ((flags & QF_OBJECTONLYONCE) && (sgThis.wasAlready))
			return false;

		if ((flags & QF_CONTEXTONLYONCE) && ((sgPrev.wasAlready) || (sgNext.wasAlready)))
			return false;
		//		if ((flags & QF_OBJECTINCONTEXTONLYONCE) && wasObjectInContext)
		//			return false;

		sgPrev.Check(sdThis, FT_CLASS);
		sgNext.Check(sdThis, FT_CLASS);

		if (!sgThis.Check(sdThis, FT_CLASS))
			return false;

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

		return true;
	}

	int GetFirstMatchingFragment(IPA* ipa, Sound** sound, LPTSTR wIn, LPTSTR wOut)
	{
		Segmentizer sgmntzr(ipa, wIn);
		Reset();

		if (!wIn)
		{
			if (sound) *sound = NULL;
			return ST_NONE;
		}

		bool isYes = false;
		while (sgmntzr.GetNext())
		{
			isYes |= Check(&sgmntzr);
			if (isYes) break;
		}

		if (!isYes)
			return ST_ERROR;

		if (sound) *sound = sgmntzr.Current();
		if (wOut) 	wcscpy(wOut, (*sound)->Symbol);

		int ret = ST_SOUND;

		if (wOut && flags & QF_ITERATE)
		{
			Condition c(sgThis.txtCondition, NULL, NULL);//вот так неловко копируем условие
			Sound* sPrev = sgmntzr.PeekPrevious(); //это пока так, ибо нет движения назад
			Sound* sNext;

			int i = 2;

			while ((sNext = sgmntzr.GetNext()) && i <= nIterate)
			{
				if (c.Check(&sgmntzr))
				{
					ret = ST_FRAGMENT;
					wcscat(wOut, sNext->Symbol);
				}
				else if (sgThis.feature[FT_CLASS] == FT_VOWEL)//проверяем потенциальные д-ги [надо это как-то обобщить]
				{
					Sound* sBase = ipa->GetBaseSound(sNext);
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
	Condition* AddCondition(LPTSTR _ftThis, LPTSTR _ftPrev, LPTSTR _ftNext, int flags = 0, LPTSTR _title = NULL, int _extraInt = 0, int _iSyllable = -1)
	{
		Condition* c = ::new Condition(_ftThis, _ftPrev, _ftNext, flags, _title, _extraInt, 2, _iSyllable);

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
	bool CheckCurrentCondition()//вроде лишнее
	{
		if (!cndCur)
			return false;

		return cndCur->Check(sgmtzr);
	}
};