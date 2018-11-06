#include "ipa.h"

#define QF_RIGHTAFTER				0x1
#define QF_RIGHTBEFORE				0x2
#define QF_FIRSTINWORD				0x100
#define QF_LASTINWORD				0x200
#define QF_OBJECTONLYONCE			0x1000
#define QF_CONTEXTONLYONCE			0x2000
#define QF_OBJECTINCONTEXTONLYONCE	0x10000


class Query
{
public:
	class Condition;//надо без этого!
	class Condition : public LinkedElement<Condition>
	{
	public:
		LPTSTR		title;
		LPTSTR		listFeature;
		int			iClass;
		Feature*	feature;
		int 		condition;

		bool		wasObject;
		bool		wasContext;
		bool		wasObjectInContext;

		Condition(int _condition, LPTSTR _listFeature, int _iClass, LPTSTR _title = NULL)
		{
			listFeature = _listFeature;
			condition = _condition;
			iClass = _iClass;
			title = _title;
			feature = NULL;

			Reset();
		}
		void Reset()
		{
			wasObject = wasContext = wasObjectInContext = false;
		}
		bool Check(Segmentizer* sgmtzr)
		{
			if (!feature)
			{
				Feature ftForSearch(listFeature/*пока одно*/);
				feature = (Feature*)sgmtzr->ipa->tFeatures.Find(&ftForSearch);//будет искать, каждый раз, если задано несущ.
			}

			SoundTable::Sound* sdThis = sgmtzr->Current(),
				*sdAdjacent;

			if (condition & QF_OBJECTONLYONCE && wasObject)
				return false;
			if (condition & QF_CONTEXTONLYONCE && wasContext)
				return false;
			if (condition & QF_OBJECTINCONTEXTONLYONCE && wasObjectInContext)
				return false;

			if (feature) if (sdThis->feature[FT_CLASS] == feature->iClass)//т.е. текущая годится как контекст в принципе
				wasContext = true;

			if (sdThis->feature[FT_CLASS] == iClass)
				wasObject = true;
			else
				return false;

			if (condition & QF_FIRSTINWORD)
			{
				if (sgmtzr->IsFirst())
					return true;
			}
			else if (condition & QF_RIGHTAFTER)
				sdAdjacent = sgmtzr->PeekPrevious();
			else if (condition & QF_RIGHTBEFORE)
				sdAdjacent = sgmtzr->PeekNext();
			if (!feature)
				return false;
			if (!sdAdjacent)
				return false;

			if (sdAdjacent->feature[FT_CLASS] == feature->iClass)
				wasObjectInContext = true;
			else
				return false;
			bool isMatch = !!(sdAdjacent->feature[feature->iType] & feature->value);
			//bool isMatch = sdAdjacent->feature[cndCur->feature->iType] & cndCur->feature->value;//даёт не то
			return isMatch;
		}
	};
	Segmentizer* sgmtzr;
	Condition* cndCur;
	LinkedList<Condition> llConditions;

	Query()
	{
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
	void AddCondition(int _condition, LPTSTR listFeature, int _iClass, LPTSTR _title = NULL)
	{
		Condition* cnd = new Condition(_condition, listFeature, _iClass, _title);
		llConditions.Add(cnd);
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
public:
};
