/*
 * Area.h
 *
 *  Created on: Mar 26, 2012
 *      Author: Gnarls
 */

#ifndef AREA_H_
#define AREA_H_

#include <map>
#include <string>

using namespace std;

class Room;
class GameWorld;

class Area
{
	public:
		Area();
		virtual ~Area();

		void						Load();
		void						Update();

	// Variables
		map<int, Room*>				m_Rooms;	// room map
		int							m_nId;		// Area id
		string						m_sName;	// Name
		string						m_sAuthor;	// Author
		string						m_sDesc;	// Description
		GameWorld *					m_pWorld;	// GameWorld

};

#endif /* AREA_H_ */
