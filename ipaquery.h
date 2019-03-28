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
	void*		dataExtra;
	int			intExtra;

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

	Condition(LPTSTR _ftThis, LPTSTR _ftPrev, LPTSTR _ftNext, int _flags = 0, LPTSTR _title = NULL, int _extraInt = 0) : sgThis(_ftThis), sgPrev(_ftPrev), sgNext(_ftNext)
	{
		title = _title;
		flags = _flags;
		intExtra = _extraInt;
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
	Condition* AddCondition(LPTSTR _ftThis, LPTSTR _ftPrev, LPTSTR _ftNext, int flags = 0, LPTSTR _title = NULL, int _extraInt = 0)
	{
		Condition* c = ::new Condition(_ftThis, _ftPrev, _ftNext, flags, _title, _extraInt);

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
	bool CheckCondition()
	{
		if (!cndCur)
			return false;

		return cndCur->Check(sgmtzr);
	}
};