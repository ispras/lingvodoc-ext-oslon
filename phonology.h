class Phonology
{
public:
	Query 	qry;

	void AddDistibutionConditions()
	{
		qry.AddCondition(L"Г", L"#", NULL, 0, L"в начале");
		qry.AddCondition(L"Г", L"ГУБ", NULL, QF_OBJECTONLYONCE, L"после губных");
		qry.AddCondition(L"Г", L"ЗУБ", NULL, QF_OBJECTONLYONCE, L"после зубных");
		qry.AddCondition(L"Г", L"ПАЛ", NULL, QF_OBJECTONLYONCE, L"после палатальных");
		qry.AddCondition(L"Г", L"ЗЯЗ", NULL, QF_OBJECTONLYONCE, L"после заднеязычных");
		qry.AddCondition(L"Г", L"ЛАР", NULL, QF_OBJECTONLYONCE, L"после ларингальных");

		qry.AddCondition(L"Г", NULL, L"ЗУБ", QF_OBJECTONLYONCE, L"перед зубными");

		qry.AddCondition(L"Г", NULL, L"ПАЛ", QF_OBJECTONLYONCE, L"перед палатальными");
		qry.AddCondition(L"Г", NULL, L"ЗЯЗ", QF_OBJECTONLYONCE, L"перед заднеязычными");
		qry.AddCondition(L"Г", NULL, L"ЛАР", QF_OBJECTONLYONCE, L"перед ларингальными");

		qry.AddCondition(L"С", L"#", NULL, 0, L"в начале");
		qry.AddCondition(L"С", NULL, L"ПЕР", QF_OBJECTONLYONCE | QF_CONTEXTONLYONCE, L"перед передними");
		qry.AddCondition(L"С", NULL, L"ЦНТ", QF_OBJECTONLYONCE | QF_CONTEXTONLYONCE, L"перед центральными");
		qry.AddCondition(L"С", NULL, L"ЗАД", QF_OBJECTONLYONCE | QF_CONTEXTONLYONCE, L"перед задними");

		qry.AddCondition(L"С", NULL, L"ОГУ", QF_OBJECTONLYONCE | QF_CONTEXTONLYONCE, L"перед огубленными");
	}



};