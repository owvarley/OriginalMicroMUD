/*
 * Room.cpp
 *
 *  Created on: Mar 26, 2012
 *      Author: Gnarls
 */

#include "Room.h"
#include "Area.h"
#include "Exit.h"
#include "../SqliteDbase.h"
#include "../Actor.h"
#include "Item.h"

using namespace std;

Room::Room()
{
	// TODO Auto-generated constructor stub
}

Room::~Room()
{
	// TODO Auto-generated destructor stub
	for (map<string, Exit*>::iterator it = m_Exits.begin(); it != m_Exits.end(); it++)
		delete (*it).second;

	m_Exits.clear();
}

void Room::AddActor(Actor * pActor)
{
	m_Actors.push_back(pActor);
}

void Room::RemActor(Actor * pActor)
{
	m_Actors.remove(pActor);
}

bool Room::HasFlag (string sFlag)
{
	if (m_Flags.find(sFlag) != m_Flags.end())
		return true;

	return false;
}

void Room::Send(const string &sMsg, ...)
{
    char buf[MAX_STRING_LENGTH*2];
    va_list args;

    va_start(args, sMsg);
    vsprintf(buf, sMsg.c_str(), args);
    va_end(args);

    std::string sEdit(buf);

	for (list<Actor*>::iterator it = m_Actors.begin(); it != m_Actors.end(); it++)
	{
		(*it)->Send(sEdit);
	}

	return;
}

// Send to a room ignoring an actor
void Room::Send(Actor * pActor, const string &sMsg, ...)
{
    char buf[MAX_STRING_LENGTH*2];
    va_list args;

    va_start(args, sMsg);
    vsprintf(buf, sMsg.c_str(), args);
    va_end(args);

    std::string sEdit(buf);

	for (list<Actor*>::iterator it = m_Actors.begin(); it != m_Actors.end(); it++)
	{
		if ((*it) == pActor)
			continue;

		(*it)->Send(sEdit);
	}

	return;
}

void Room::Load()
{
	m_sName = SqliteDbase::Get()->GetString("roomdata", "name", "roomid", m_nId);
	m_sDesc = SqliteDbase::Get()->GetString("roomdata", "desc", "roomid", m_nId);
	// Strip any escape characters (\') from the string
	size_t found;
	found = m_sDesc.find("\\'");

	while (found != string::npos)
	{
		m_sDesc.replace(found, 2, "'");
		found = m_sDesc.find("\\'");
	}

	m_nTerrain = SqliteDbase::Get()->GetInt("roomdata", "terrain", "roomid", m_nId);
	m_nX = SqliteDbase::Get()->GetInt("roomdata", "x", "roomid", m_nId);
	m_nY = SqliteDbase::Get()->GetInt("roomdata", "y", "roomid", m_nId);
	m_nZ = SqliteDbase::Get()->GetInt("roomdata", "z", "roomid", m_nId);

	// Load Normal Exits
	CppSQLite3Query query = SqliteDbase::Get()->GetQuery("exitdata", "roomfrom", m_nId);

	while (!query.eof())
	{
		Exit * pExit = new Exit();
		pExit->m_nId = query.getIntField("exitid");
		pExit->m_nTo = query.getIntField("roomto");
		pExit->m_sDir = query.getStringField("direction");

		// Load Exit flags
		CppSQLite3Query query2 = SqliteDbase::Get()->GetQuery("exitflags", "exitid", pExit->m_nId);

		while (!query2.eof())
		{
			pExit->m_Flags.insert(make_pair(query2.getStringField("flag"), 1));
			query2.nextRow();
		}

		m_Exits.insert(make_pair(pExit->m_sDir, pExit));
		query.nextRow();
	}

	// Load Automatic Exits
	query = SqliteDbase::Get()->GetQuery("SELECT * from roomdata WHERE (((x>=%d and x<=%d) and (y>=%d and y<=%d) and (z=%d) or (x=%d and y=%d and z>=%d and z<=%d)) and roomid NOT IN (SELECT roomid FROM exitskip where roomid=%d));",
			m_nX - 1, m_nX + 1, m_nY - 1, m_nY + 1, m_nZ, m_nX, m_nY, m_nZ - 1, m_nZ + 1, m_nId);

	// Query now contains a list of rooms that we should automatically be linked to
	while (!query.eof())
	{
		// Skip the room we are in
		if (query.getIntField("roomid") == m_nId)
		{
			query.nextRow();
			continue;
		}

		Exit * pExit = new Exit();
		//pExit->m_nId = query.getIntField("exitid");
		pExit->m_nTo = query.getIntField("roomid");
		pExit->m_sDir = Main::CardinalDirection(m_nX, m_nY, m_nZ, query.getIntField("x"), query.getIntField("y"), query.getIntField("z"));

		m_Exits.insert(make_pair(pExit->m_sDir, pExit));
		query.nextRow();
	}

	// Load any Items or NPCs that have been reset to this room
	query = SqliteDbase::Get()->GetQuery("select * from resetdata where roomid=%d", m_nId);

	while (!query.eof())
	{
		int nNum = query.getIntField("number");
		if (StrEquals(query.getStringField("type"), "NPC"))
		{
			while (--nNum >= 0)
			{
				Actor * pNPC = new Actor;
				pNPC->m_nId  = query.getIntField("typeid");
				pNPC->m_nType = _NPC;
				pNPC->m_pWorld = this->m_pArea->m_pWorld;
				pNPC->LoadNPC();
				pNPC->m_pRoom = this;
				m_Actors.push_back(pNPC);	// Add to room
			}
		}
		else if (StrEquals(query.getStringField("type"), "Item"))
		{
			while (--nNum >= 0)
			{
				Item * pItem = new Item;
				pItem->m_nId = query.getIntField("typeid");
				pItem->Load();
				m_Items.push_back(pItem);
			}
		}
		query.nextRow();
	}


	// SELECT * FROM roomdata where (((x>=".($x-1)." and x<=".($x+1).") and (y>=".($y-1)." and y<=".($y+1).") and (z=".$z.")) or (x=".$x." and y=".$y." and z>=".($z-1)." and z<=".($z+1).")) and roomid NOT IN (SELECT roomidto FROM exitskip where roomid=".$roomid.");";
}

void Room::Update()
{
	list<Item*> DeleteList;

	// Update any items in the room
	for (list<Item*>::iterator it = m_Items.begin(); it != m_Items.end(); it ++)
	{
		(*it)->Update();

		// Has this item now expired?
		if ((*it)->m_Variables->HasVar("expired"))
		{
			if ((*it)->m_Variables->Var("type") == "bloodtrail")
				Send("%s dries up.\n\r", (*it)->m_sName.c_str());
			else if ((*it)->m_Variables->Var("type") == "corpse")
				Send("%s rots away into dust.\n\r", (*it)->m_sName.c_str());
			else
				Send("%s decays away into dust.\n\r", (*it)->m_sName.c_str());

			DeleteList.push_back((*it));
		}
	}

	for (list<Item*>::iterator it = DeleteList.begin(); it != DeleteList.end(); it ++)
	{
		m_Items.remove(*it);
		delete (*it);
	}


}
