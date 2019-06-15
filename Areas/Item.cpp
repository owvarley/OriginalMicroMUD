/*
 * Item.cpp
 *
 *  Created on: Mar 26, 2012
 *      Author: Gnarls
 */

#include "Item.h"
#include "../SqliteDbase.h"
#include "../Variable.h"

using namespace std;

Item::Item()
{
	m_Variables = new VariableManager();
}

Item::~Item()
{
	delete m_Variables;
}

void Item::Load()
{
	if (m_nId == 0)
		return;

	CppSQLite3Query query  = SqliteDbase::Get()->GetQuery("select * from itemdata where itemid=%d", m_nId);

	while (!query.eof())
	{
		m_sName = query.getStringField("name");
		query.nextRow();
	}

	query = SqliteDbase::Get()->GetQuery("select variable, value, number from itemvariables where itemid=%d", m_nId);
	int nQualHi = 0;
	int nQualLo = 0;
	while (!query.eof())
	{
		// Column 0 == Variable
		// Column 1 == Value
		// Column 2 == Number (If == 1 then this is a number)
		if (StrEquals(query.fieldValue(0), "quality_lo"))
		{
			nQualLo = atoi(query.fieldValue(1));
		}
		else if (StrEquals(query.fieldValue(0), "quality_hi"))
		{
			nQualHi = atoi(query.fieldValue(1));
		}
		else
		{
			if (atoi(query.fieldValue(2)) > 0)
				m_Variables->AddVar(query.fieldValue(0), atoi(query.fieldValue(1)));
			else
				m_Variables->AddVar(query.fieldValue(0), query.fieldValue(1));
		}

		query.nextRow();
	}

	// Load in item flags
	query = SqliteDbase::Get()->GetQuery("select flag from itemflags where itemid=%d", m_nId);

	while (!query.eof())
	{
		m_Flags[ToLower(query.fieldValue(0))] = 1;
		query.nextRow();
	}

	if (nQualLo == 0 && nQualHi == 0)
		nQualLo = nQualHi = 1;

	// Determine quality
	int nQual = Main::rand_range(nQualLo, nQualHi);
	m_Variables->AddVar("curr_quality", nQual);
	m_Variables->AddVar("max_quality", nQual);

	return;
}

bool Item::HasFlag(string sVar)
{
	if (m_Flags.find(ToLower(sVar)) != m_Flags.end())
		return true;

	return false;
}

void Item::AddFlag(string sVar)
{
	if (HasFlag(ToLower(sVar)))
		return;

	m_Flags[ToLower(sVar)] = 1;
	return;
}

void Item::RemFlag(string sVar)
{
	if (HasFlag(ToLower(sVar)))
		m_Flags.erase(sVar);

	return;
}

void Item::Update()
{
	// Check for a decay timer
	if (m_Variables->HasVar("decay_timer"))
	{
		m_Variables->AddToVar("decay_timer", -PULSE_AREA);

		if (m_Variables->VarI("decay_timer") <= 0)
			m_Variables->Set("expired", 1);
	}
}
