/*
 * Actor.h
 *
 *  Created on: Mar 25, 2012
 *      Author: Gnarls
 */

#ifndef ACTOR_H_
#define ACTOR_H_

#include <string>
#include <list>
#include <map>
#include "GameWorld.h"
#include "Areas/Room.h"
#include "Variable.h"

using namespace std;

class Room;
class Affect;
class Item;
class GameWorld;
class Combat;
class VariableManager;

class Actor
{
	public:
		Actor();
		virtual ~Actor();

		virtual void				Send(const string&, ...);
		void 						Act(string output, string output2, Actor * pVictim, Item * pObj, Item * pObj2, string sMessage, ...);
		void 						Act(string sMessage, ...);
		void 						Act(Actor * pVictim, string sMessage, ...);
		virtual bool				Move(string sDir);

		// Affect handling
		// Add an affect with no time limit
		void						AddAffect(string sAff);
		// Add an affect with a given location, modifier and timelimit
		void						AddAffect(string sAff, int nLocation, int nModifier, int nM, int nS);
		// Add an affect with given time delay but no location or modifier
		void						AddAffect(string sAff, int nM, int nS);
		void						RemAffect(string sAff);
		void						IncreaseAffect(string sAff, int nMod);
		void						IncreaseTime(string sAff, int nM, int nS);
		int							AffectMod(string sAff);
		bool						IsAffected(string sAff);
		// Room Description
		string						RoomDesc(Actor * pLooker, bool bHidden);
		// Health
		void						Heal (int nAmount);
		bool						Damage (int nAmount);
		// Death
		void						DeathCry(Actor * pKiller, bool bDefeat = false);
		void						Die(Actor * pKiller, bool bSilent = false);
		void						CreateCorpse(bool bDefeat = false);
		void						DetermineLoot();
		void						Respawn();
		// Weapon/Armour
		int							Armour(string sVal);
		Item *						GetArmour() { return GetEquipped("body"); };
		int							Weapon(string sVal);
		Item * 						GetWeapon() { return GetEquipped("hands"); };
		void						DamageItem(string sWearloc, int nAmount);
		Item *						GetEquipped(string sWearloc);

		// Perception
		string						Perception(Actor * pVictim);

		// Attribute handling
		int							Attribute(string nAttr);
		int							AttrMax(string nAttr);

		// Combat
		void						CombatMessage(Actor * pTarget, bool bDeath);
		bool						IsFighting();

		// NPC handling
		void						LoadNPC();

		// Updates
		void						Update();

		// Flags
		bool						HasFlag(string sFlag);
		void						AddFlag(string sFlag);
		void						RemFlag(string sFlag);

		// Data Variables
		int							m_nId;
		string						m_sName;
		string						m_sDesc;
		int							m_nType;
		map<string, Item*>			m_Equipment;
		list<Item*>					m_Inventory;
		map<string, int>			m_Tags;
		map<string, time_t>			m_TagTimer;
		VariableManager *			m_Variables;
		//map<string, Variable*> 		m_Variables;
		map<string, int>			m_Flags;
		map<int, int>				m_Loot;			// Loot table for NPCs only
		map<string, Affect*>		m_Affects;

		// Reference Variables
		Actor *						m_pFighting;
		Actor * 					m_pTarget;
		Room * 						m_pRoom;
		GameWorld *					m_pWorld;
		Combat *					m_pCombat;
};

#endif /* ACTOR_H_ */
