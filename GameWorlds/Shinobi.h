/*
 * Shinobi.h
 *
 *  Created on: Mar 25, 2012
 *      Author: Gnarls
 */

#ifndef SHINOBI_H_
#define SHINOBI_H_

#include "../GameWorld.h"

using namespace std;

class GameSocket;
class Actor;
class Player;

class Shinobi : public GameWorld
{
	public:
		Shinobi();
		virtual ~Shinobi();

		void 				Update();
		void				Interpret(GameSocket * pSocket, string sInput);
		void				PlayMenu(GameSocket * pSocket);
		void				Splash(GameSocket * pSocket);
		void				Nanny(GameSocket * pSocket, string sInput);
		void				LoadAreas();
		string				RandomName();
		Room *				StartingRoom();
		void				Send(const string&, ...);
		void				Console(const string&, ...);
		void				CreatePlayer(Player * pActor);
		void				CombatManager();

		// Deleting a Player
		void				DeletePlayer(Player * pPlayer);

		void				SendMSSP(GameSocket * pSocket);

		// Anchillary functions
		string				Health(int nCurr, int nMax);
		// Attributes
		string				AttrName (int nId);
		// Affects
		string				AffectName (int nAffect);

		typedef bool (Shinobi::*InputCmd) (Actor*, string);


		// COMMANDS
		bool				Command(Actor * pActor, string sCommand, std::string sArguments);
		// Global
		bool				CmdQuit(Actor * pActor, string sArguments);
		bool				CmdWho(Actor * pActor, string sArguments);
		bool				CmdWhois(Actor * pActor, string sArguments);
		bool				CmdTime(Actor * pActor, string sArguments);
		// Communication
		bool				CmdSay(Actor * pActor, string sArguments);
		bool				CmdOsay(Actor * pActor, string sArguments);
		bool				CmdShout(Actor * pActor, string sArguments);
		bool				CmdEmote(Actor * pActor, string sArguments);
		bool				CmdChat(Actor * pActor, string sArguments);
		// World
		bool				CmdLook(Actor * pActor, string sArguments);
		// Character
		bool				CmdTitle(Actor * pActor, string sArguments);
		bool				CmdDesc(Actor * pActor, string sArguments);
		bool				CmdScore(Actor * pActor, string sArguments);
		bool				CmdTrain(Actor * pActor, string sArguments);
		bool				CmdSave(Actor * pActor, string sArguments);
		bool				CmdAffects(Actor * pActor, string sArguments);
		// Inventory and Interaction
		bool				CmdGet(Actor * pActor, string sArguments);
		bool				CmdDrop(Actor * pActor, string sArguments);
		bool				CmdWear(Actor * pActor, string sArguments);
		bool				CmdRemove(Actor * pActor, string sArguments);
		bool				CmdEquipment(Actor * pActor, string sArguments);
		bool				CmdInventory(Actor * pActor, string sArguments);
		bool				CmdExamine(Actor * pActor, string sArguments);
		// Shinobi Commands
		bool				CmdSneak(Actor * pActor, string sArguments);
		bool				CmdMeditate(Actor * pActor, string sArguments);
		bool				CmdHide(Actor * pActor, string sArguments);
		bool				CmdVisible(Actor * pActor, string sArguments);
		bool				CmdFocus(Actor * pActor, string sArguments);
		bool				CmdSuicide(Actor * pActor, string sArguments);
		bool				CmdRetire(Actor * pActor, string sArguments);
		bool				CmdPlaydead(Actor * pActor, string sArguments);
		bool				CmdThrow(Actor * pActor, string sArguments);
		// Combat
		bool				CmdKill(Actor * pActor, string sArguments);
		bool				CmdFight(Actor * pActor, string sArguments);
		bool				CmdStance(Actor * pActor, string sArguments);
		// Combat Moves
		bool				CmdSmokebomb(Actor * pActor, string sArguments);
		bool				CmdCombatMove(Actor * pActor, string sArguments);


		map<string, int>			m_Names;		// Random name generator, normal names
		map<string, int>			m_Honorific;	// Random name generator, honorific names
		map<string, InputCmd>		m_Commands;		// Commands
		map<string, int> 			m_PreCommands;	// Commands that require input word preservation
};

#endif /* SHINOBI_H_ */
