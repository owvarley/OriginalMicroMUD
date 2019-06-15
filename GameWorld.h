/*
 * GameWorld.h
 *
 *  Created on: Mar 15, 2012
 *      Author: Gnarls
 */

#ifndef GAMEWORLD_H_
#define GAMEWORLD_H_

#include <string>
#include <vector>
#include <list>
#include <map>

class Area;
class Player;
class Room;
class Item;
class Actor;
class GameSocket;
class VariableManager;
class Variable;
class Combat;
class CombatMessage;

using namespace std;

class GameWorld
{

	public:
		GameWorld();
		virtual ~GameWorld();
		int	NumPlayers() { return m_Players.size(); };	// Number of Players online

		virtual void				Interpret(GameSocket * pSocket, string sInput);
		//virtual void				CombatManager();
		virtual void				Update();
		virtual void				PlayMenu(GameSocket * pSocket);
		virtual void				Splash(GameSocket * pSocket);
		virtual void				Nanny(GameSocket * pSocket, string sInput);
		bool						LoadCombatMessages();
		virtual void				LoadAreas();
		virtual Room * 				StartingRoom();
		virtual bool				Move(GameSocket * pSocket, string sDirection);

		// Functions
		virtual void				DeletePlayer (Player * pActor);
		virtual void				AddPlayer (Player * pActor);
		virtual void				RemPlayer (Player * pActor);
		virtual void				CreatePlayer (Player * pActor);

		Item * 						FindItem (string sName, Room * pRoom);
		Item * 						FindItem (string sName, Actor * pActor);
		Item *						FindItem (string sName, Item * pItem);

		virtual void				Send(const string sMsg, ...);
		virtual void				Send(Actor * pActor, const string sMsg, ...);
		virtual void				Console(const string sMsg, ...);

		virtual void				SendMSSP (GameSocket * pSocket);

		virtual bool				Command(Actor * pActor, string sCommand, string sArguments);
		virtual void				CombatManager();
		string						CombatMsg(string sMove, string sWeapon = "", string sState = "");
		void						SendCombatMsg(Actor * pActor, Actor * pVictim, string sMove, string sState = "");

		// Generic Commands
		bool						CmdRoll(Actor * pActor, string sArguments);

		// Attributes
		virtual string				AttrName (int nId);

		// Affects
		virtual string				AffectName (int nAffect);

		// Variables
		string						m_sName;		// Name
		string						m_sDesc;		// Description of Game
		vector<Combat*>				m_Combat;		// Active PvP fights
		vector<CombatMessage*> 		m_CombatMsgs;	// List of Combat Messages
		map<int, Player*>       	m_Players;		// Map of players
		map<int, Area*>				m_Areas;		// Loaded areas
		map<int, Item*>				m_Items;		// Item database
		map<int, Actor*>			m_NPCs;			// NPC database
		VariableManager	*			m_Variables;	// Variables
		//map<string, Variable*>  	m_Variables;	// Variables
		map<string, string>			m_Attributes;	// Player Attributes
		map<string, string>			m_Colours;		// Custom Colour codes

};

#endif /* GAMEWORLD_H_ */
