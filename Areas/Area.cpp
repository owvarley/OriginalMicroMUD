/*
 * Area.cpp
 *
 *  Created on: Mar 26, 2012
 *      Author: Gnarls
 */

#include "Area.h"
#include "../SqliteDbase.h"
#include "Room.h"

Area::Area()
{
	// TODO Auto-generated constructor stub

}

Area::~Area()
{
	// TODO Auto-generated destructor stub
}

// Area update function
void Area::Update()
{
	for (map<int,Room*>::iterator it = m_Rooms.begin(); it != m_Rooms.end(); it++)
		(*it).second->Update();
}

// Load the area from the database using the preset Area ID
void Area::Load()
{
	m_sName = SqliteDbase::Get()->GetString("areadata", "name", "areaid", m_nId);
	m_sDesc = SqliteDbase::Get()->GetString("areadata", "desc", "areaid", m_nId);
	m_sAuthor = SqliteDbase::Get()->GetString("areadata", "author", "areaid", m_nId);

	CppSQLite3Query query = SqliteDbase::Get()->GetQuery("arearooms", "areaid", m_nId);

	while (!query.eof())
	{
		Room * pRoom = new Room();
		pRoom->m_nId = query.getIntField("roomid");
		pRoom->m_pArea = this;
		pRoom->Load();
		m_Rooms.insert(make_pair(pRoom->m_nId, pRoom));
		query.nextRow();
	}
}
