/*
 * Player.cpp
 *
 *  Created on: Mar 25, 2012
 *      Author: Gnarls
 */

#include "Player.h"
#include "Main.h"
#include <string>
#include <stdarg.h>
#include "GameSocket.h"
#include "SqliteDbase.h"

Player::Player()
{
	// TODO Auto-generated constructor stub
	m_nType = _PLAYER;

	// Default fields
	m_Variables->AddVar("curr_hp", 	100);
	m_Variables->AddVar("max_hp", 	100);
	m_Variables->AddVar("kills", 	0);
	m_Variables->AddVar("npc_kills", 0);
	m_Variables->AddVar("deaths", 	0);
	m_Variables->AddVar("desc",    "");
	m_Variables->AddVar("title",   "");
}

Player::~Player()
{

}

void Player::Save ()
{
	for (map<string, Variable*>::iterator it = m_Variables->m_Variables.begin(); it != m_Variables->m_Variables.end(); it++)
	{
		if ((*it).second->m_bInt)
			Save ((*it).first, (*it).second->m_nValue);
		else
			Save ((*it).first, (*it).second->m_sValue);
	}
}

void Player::Save (string sVariable, int sValue)
{
	int nResult = SqliteDbase::Get()->GetInt("select count(playerid) from playervariables where variable like '%s' and playerid=%d", sVariable.c_str(), m_nId);

	// If the value already exists, replace it, if not, add it
	if (nResult <= 0)
	{
		stringstream sSQL;
		sSQL << "insert into playervariables (playerid, variable, value, number) values (" << m_nId << ", '" << sVariable << "', '" << sValue << "', 1);";
		SqliteDbase::Get()->ExecDML(sSQL.str().c_str());
	}
	else
	{
		stringstream sSQL;
		sSQL << "update playervariables set value=" << sValue << " where playerid=" << m_nId << " and variable='" << sVariable << "';";
		SqliteDbase::Get()->ExecDML(sSQL.str().c_str());
	}

	return;
}

void Player::Save (string sVariable, string sValue)
{
	int nResult = SqliteDbase::Get()->GetInt("select count(playerid) from playervariables where variable like '%s' and playerid=%d", sVariable.c_str(), m_nId);

	// If the value already exists, replace it, if not, add it
	if (nResult <= 0)
	{
		stringstream sSQL;
		sSQL << "insert into playervariables (playerid, variable, value, number) values (" << m_nId << ", '" << sVariable << "', '" << sValue << "', 0);";
		SqliteDbase::Get()->ExecDML(sSQL.str().c_str());
	}
	else
	{
		stringstream sSQL;
		sSQL << "update playervariables set value='" << sValue << "' where playerid=" << m_nId << " and variable='" << sVariable << "';";
		SqliteDbase::Get()->ExecDML(sSQL.str().c_str());
	}

	return;
}

// Load a Player up from the Database
void Player::Load()
{
	CppSQLite3Query query = SqliteDbase::Get()->GetQuery("select variable, value, number from playervariables where playerid=%d", m_nId);

	while (!query.eof())
	{
		// Column 0 == Variable
		// Column 1 == Value
		// Column 2 == Number (If == 1 then this is a number)

		if (atoi(query.fieldValue(2)) > 0)
			m_Variables->AddVar(query.fieldValue(0), atoi(query.fieldValue(1)));
		else
			m_Variables->AddVar(query.fieldValue(0), query.fieldValue(1));

		query.nextRow();
	}
}

// Send a message to a player
void Player::Send(const string &s, ...)
{
    char buf[MAX_STRING_LENGTH*2];
    va_list args;

    va_start(args, s);
    vsprintf(buf, s.c_str(), args);
    va_end(args);

    std::string sEdit(buf);

    if (m_pSocket)
    	m_pSocket->Send(sEdit);

    return;
}
