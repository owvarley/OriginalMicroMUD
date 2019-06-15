/*
 * Player.h
 *
 *  Created on: Mar 25, 2012
 *      Author: Gnarls
 */

#ifndef PLAYER_H_
#define PLAYER_H_

#include "Actor.h"
#include <string>
#include <map>

// Forward declaration
class GameSocket;
class Account;

class Player : public Actor
{
	public:
		Player();
		virtual ~Player();

		void					Send(const std::string&, ...);
		void					Save(string sVariable, string sValue);
		void					Save(string sVariable, int nValue);
		void					Save();
		void					Load();

	// Variables
		Account *					m_pAccount;		// The account that owns this char
		GameSocket * 				m_pSocket;		// The socket 'playing' this char

};

#endif /* PLAYER_H_ */
