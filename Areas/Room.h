/*
 * Room.h
 *
 *  Created on: Mar 26, 2012
 *      Author: Gnarls
 */

#ifndef ROOM_H_
#define ROOM_H_

#include <string>
#include <map>
#include <list>

using namespace std;

class Actor;
class Area;
class Exit;
class Item;

class Room
{
	public:
		Room();
		virtual ~Room();

		void					Load();
		void					Send(const std::string&, ...);
		void					Send(Actor * pActor, const std::string&, ...);
		string					Location();
		void					AddActor(Actor * pActor);
		void					RemActor(Actor * pActor);
		void					Update();

		bool					HasFlag(string sFlag);

	// Variables
		int						m_nId;
		string					m_sName;
		string					m_sDesc;
		int						m_nTerrain;
		int						m_nX;
		int						m_nY;
		int						m_nZ;
		map<string, int>		m_Flags;
		map<string, Exit*>		m_Exits;
		Area *					m_pArea;
		list<Actor*>			m_Actors;
		list<Item*>				m_Items;

};

#endif /* ROOM_H_ */
