/*
 * Combat.h
 *
 *  Created on: Sep 19, 2012
 *      Author: owen
 */

#ifndef COMBAT_H_
#define COMBAT_H_

#include <string>
#include <vector>

#define ATTACKER 0
#define VICTIM   1
#define STRIKE_FUMBLE -2
#define STRIKE_MISS   -1

using namespace std;

class Actor;

class Combat
{
	public:
		Combat();
		virtual ~Combat();

	// Functions
		virtual void		Update();	// Run a round of combat
		void				Send(string sMes, ...);
		virtual void		End();
		void				ResetStrike();
		void				Reset(Actor * pActor);


	// Variables
		Actor * 			m_pAttacker;	// Player that initiated combat
		Actor *				m_pDefender;	// Player that is being attacked
		int					m_nRounds;		// Number of rounds played
		bool				m_bEnded;		// Has combat ended?
		bool				m_bResist;		// Was the strike resisted?
		bool				m_bBlock;		// Was the strike blocked?
		int					m_nDamage;		// Did the strike deal damage?
		vector<string>		m_Moves;		// Valid combat moves

};

class CombatMessage
{
	public:
	CombatMessage(string sMove, string sWeapon, string sState, string sMessage);
	virtual ~CombatMessage();

	// Variables
		string				m_sMessage;			// The string
		string				m_sMove;			// The move this message is for
		string				m_sState;			// Hit, Miss, etc
		string				m_sWeapon;			// Additional information
};

#endif /* COMBAT_H_ */
