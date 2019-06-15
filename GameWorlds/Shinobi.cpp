/*
 * Shinobi.cpp
 *
 *  Created on: Mar 25, 2012
 *      Author: Gnarls
 */

#include "Shinobi.h"
#include <iostream>
#include <time.h>
#include <arpa/telnet.h>
#include "../SqliteDbase.h"
#include "../Libraries/tinyxml2.h"
#include "../GameSocket.h"
#include "../Account.h"
#include "ShinobiCombat.h"
#include "../Player.h"
#include "../Affect.h"
#include "../Variable.h"
#include "../Areas/Area.h"
#include "../Areas/Item.h"
#include "../Areas/Exit.h"
#include "../Areas/Room.h"
#include "../Protocol.h"
#include "../Combat.h"
#include "../Main.h"
#include "../GameServer.h"

using namespace std;
using namespace tinyxml2;

Shinobi::Shinobi()
{
	// Load random name generator
	XMLDocument doc;
	doc.LoadFile( "data/names.xml" );

	if (doc.Error())
	{
		doc.PrintError();
	}
	else
	{
		XMLElement * pText = doc.FirstChildElement( "name" );

		// Parse the XML file for data
		while ( pText )
		{
			m_Names[pText->GetText()] = 1;
			pText = pText->NextSiblingElement( "name" );
		}

		pText = doc.FirstChildElement( "honorific" );

		while ( pText )
		{
			m_Honorific[pText->GetText()] = 1;
			pText = pText->NextSiblingElement( "honorific" );
		}
	}

	// Initialise Commands
	m_Commands["who"] 	= &Shinobi::CmdWho;
	m_Commands["whois"] = &Shinobi::CmdWhois;
	m_Commands["chat"] 	= &Shinobi::CmdChat;
	m_Commands["look"] 	= &Shinobi::CmdLook;
	m_Commands["quit"] 	= &Shinobi::CmdQuit;
	m_Commands["ooc"]  	= &Shinobi::CmdChat;
	m_Commands["time"] 	= &Shinobi::CmdTime;
	m_Commands["say"]	= &Shinobi::CmdSay;
	m_Commands["osay"]	= &Shinobi::CmdOsay;
	m_Commands["shout"]	= &Shinobi::CmdShout;
	m_Commands["emote"]	= &Shinobi::CmdEmote;
	m_Commands["title"] = &Shinobi::CmdTitle;
	m_Commands["desc"] 	= &Shinobi::CmdDesc;
	m_Commands["score"] = &Shinobi::CmdScore;
	m_Commands["train"] = &Shinobi::CmdTrain;
	m_Commands["save"]  = &Shinobi::CmdSave;
	m_Commands["roll"]  = &GameWorld::CmdRoll;

	m_Commands["get"]  		 = &Shinobi::CmdGet;
	m_Commands["drop"]  	 = &Shinobi::CmdDrop;
	m_Commands["wear"]  	 = &Shinobi::CmdWear;
	m_Commands["remove"]  	 = &Shinobi::CmdRemove;
	m_Commands["examine"] 	 = &Shinobi::CmdExamine;
	m_Commands["target"] 	 = &Shinobi::CmdLook;
	m_Commands["equipment"]  = &Shinobi::CmdEquipment;
	m_Commands["inventory"]  = &Shinobi::CmdInventory;
	// Shinobi commands
	m_Commands["sneak"]  	= &Shinobi::CmdSneak;
	m_Commands["meditate"]  = &Shinobi::CmdMeditate;
	m_Commands["hide"]  	= &Shinobi::CmdHide;
	m_Commands["visible"] 	= &Shinobi::CmdVisible;
	m_Commands["focus"] 	= &Shinobi::CmdFocus;
	m_Commands["retire"] 	= &Shinobi::CmdRetire;
	m_Commands["suicide"]  	= &Shinobi::CmdSuicide;
	m_Commands["affects"]  	= &Shinobi::CmdAffects;
	m_Commands["playdead"] 	= &Shinobi::CmdPlaydead;
	m_Commands["throw"] 	= &Shinobi::CmdThrow;
	// Combat
	m_Commands["kill"]		= &Shinobi::CmdKill;
	m_Commands["fight"]		= &Shinobi::CmdFight;
	m_Commands["target"]	= &Shinobi::CmdLook;
	m_Commands["stance"]	= &Shinobi::CmdStance;
	// Combat Moves
	m_Commands["smokebomb"]	= &Shinobi::CmdSmokebomb;
	m_Commands["strike"]	= &Shinobi::CmdCombatMove;
	m_PreCommands["strike"] = 1;
	m_Commands["block"]		= &Shinobi::CmdCombatMove;
	m_PreCommands["block"] = 1;
	m_Commands["dodge"]		= &Shinobi::CmdCombatMove;
	m_PreCommands["dodge"] = 1;
	m_Commands["feint"]		= &Shinobi::CmdCombatMove;
	m_PreCommands["feint"] = 1;
	m_Commands["surge"]		= &Shinobi::CmdCombatMove;
	m_PreCommands["surge"] = 1;
	m_Commands["flee"]		= &Shinobi::CmdCombatMove;
	m_PreCommands["flee"] = 1;

}

Shinobi::~Shinobi()
{
	// Free memory
	for (vector<CombatMessage*>::iterator it = m_CombatMsgs.begin(); it != m_CombatMsgs.end(); it ++)
		delete (*it);

	m_CombatMsgs.clear();
}

// Random Name Generator
// Last updated: 1/10/12
// There are two types of names: Honorifics and Names. Each organized along its own lists.
// When generating a random name a random formula is first selected:
// 	1. Name
// 	2. Name + Name
// 	3. Honorific + Name
// 	4. Honorific + Name + Name
//
// Using the above formula two or more random elements are strung together.
// No element should be permitted to repeat itself.
//
string Shinobi::RandomName()
{
	int nNum = Main::rand_range(1, 4);	// Determine the format of our name
	stringstream sName;
	map<string, int> m_rNames;

	// Select a random element
	map<string, int>::iterator prev;
	map<string, int>::iterator it;

	// Check and add a Honorific at the front if required
	if (nNum == 3 || nNum == 4)
	{
		it = m_Honorific.begin();
		advance (it, Main::rand_range(0, m_Honorific.size()));
		sName << (*it).first << " ";	// Honorifics always have a space after
	}

	// Append a single name
	 it = m_Names.begin();
	 advance ( it, Main::rand_range(0, m_Names.size()));
	 prev = it;
	 sName << (*it).first;

	// Check if we need to add a second name
	if (nNum == 2 || nNum == 4)
	{
		while (prev == it)
		{
			prev = m_Names.begin();
			advance ( prev, Main::rand_range(0, m_Names.size()));
		}

		sName << " " << (*prev).first;
	}

	return sName.str();
}


void Shinobi::Update()
{

}

void Shinobi::SendMSSP (GameSocket * pSocket)
{
	char MSSP[MAX_MSSP_BUFFER];
	stringstream ssStr;

	ssStr << this->NumPlayers();

	// Kick off with IAC SB MSSP
	sprintf( MSSP, "%c%c%c", (char)IAC, (char)SB, TELOPT_MSSP);

	// Add all the variables
	// Required
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "NAME", 			MSSP_VAL, "Shinobi");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "PLAYERS", 		MSSP_VAL, ssStr.str().c_str());
	// TODO Implement this
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "UPTIME", 			MSSP_VAL, "0");
	// Generic
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "CRAWL DELAY", 	MSSP_VAL, "-1");
	// Extra
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "HOSTNAME",		MSSP_VAL, "micromud.mudhosting.net");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "PORT",			MSSP_VAL, "6660");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "CODEBASE",		MSSP_VAL, "MicroMud");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "CONTACT",	 		MSSP_VAL, "owen@micromud.org");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "CREATED",	 		MSSP_VAL, "2012");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "IP",	 			MSSP_VAL, "66.85.147.90");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "LANGUAGE",		MSSP_VAL, "English");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "LOCATION",		MSSP_VAL, "UK");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "MINIMUM AGE",		MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "WEBSITE",	 		MSSP_VAL, "www.micromud.org");

	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "FAMILY",	 		MSSP_VAL, "Custom");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "GENRE",	 		MSSP_VAL, "");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "GAMEPLAY",		MSSP_VAL, "Player versus Player");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "STATUS",	 		MSSP_VAL, "Alpha");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "GAMESYSTEM",		MSSP_VAL, "Custom");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "INTERMUD",		MSSP_VAL, "");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "SUBGENRE",		MSSP_VAL, "");

	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "AREAS",	 		MSSP_VAL, "2");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "HELPFILES",		MSSP_VAL, "100");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "MOBILES",	 		MSSP_VAL, "15");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "OBJECTS",	 		MSSP_VAL, "26");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "ROOMS",	 		MSSP_VAL, "25");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "CLASSES",	 		MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "LEVELS",	 		MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "RACES",	 		MSSP_VAL, "1");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "SKILLS",	 		MSSP_VAL, "0");

	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "ANSI",	 		MSSP_VAL, "1");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "GMCP",	 		MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "MCCP",	 		MSSP_VAL, "1");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "MCP",	 			MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "MSDP",	 		MSSP_VAL, "1");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "MSP",	 			MSSP_VAL, "1");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "MXP",	 			MSSP_VAL, "1");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "PUEBLO",	 		MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "UTF-8",	 		MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "VT100",	 		MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "XTERM 256 COLORS",MSSP_VAL, "1");

	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "PAY TO PLAY", 	MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "PAY FOR PERKS", 	MSSP_VAL, "0");

	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "HIRING BUILDERS",MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "HIRING CODERS", 	MSSP_VAL, "0");

	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "ADULT MATERIAL", 	MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "MULTICLASSING", 	MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "NEWBIE FRIENDLY",MSSP_VAL, "1");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "PLAYER CITIES", 	MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "PLAYER CLANS", 	MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "PLAYER CRAFTING",MSSP_VAL, "1");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "PLAYER GUILDS", 	MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "EQUIPMENT SYSTEM",MSSP_VAL, "1");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "MULTIPLAYING", 	MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "PLAYERKILLING", 	MSSP_VAL, "1");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "QUEST SYSTEM", 	MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "ROLEPLAYING", 	MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "TRAINING SYSTEM",MSSP_VAL, "1");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "WORLD ORIGINALITY",MSSP_VAL, "1");

	// Finish the text with IAC SE
	sprintf( MSSP, "%c%c", (char)IAC, (char) SE);

	pSocket->Send("%s\0", MSSP);
}

// Display the Welcome screen for Shinobi
void Shinobi::Splash(GameSocket * pSocket)
{
	//	 _______          _________ _        _______  ______  _________
	//	(  ____ \|\     /|\__   __/( (    /|(  ___  )(  ___ \ \__   __/
	//	| (    \/| )   ( |   ) (   |  \  ( || (   ) || (   ) )   ) (
	//	| (_____ | (___) |   | |   |   \ | || |   | || (__/ /    | |
	//	(_____  )|  ___  |   | |   | (\ \) || |   | ||  __ (     | |
	//	      ) || (   ) |   | |   | | \   || |   | || (  \ \    | |
	//	/\____) || )   ( |___) (___| )  \  || (___) || )___) )___) (___
	//	\_______)|/     \|\_______/|/    )_)(_______)|/ \___/ \_______/
	//                                                KILL OR BE KILLED
	//                                   A Game by Ken Mikkelson (2012)
	//                                       Implemented by Owen Varley

	pSocket->Send("	 _______          _________ _        _______  ______  _________ \n\r");
	pSocket->Send("	(  ____ \\|\\     /|\\__   __/( (    /|(  ___  )(  ___ \\ \\__   __/ \n\r");
	pSocket->Send("	| (    \\/| )   ( |   ) (   |  \\  ( || (   ) || (   ) )   ) (    \n\r");
	pSocket->Send("	| (_____ | (___) |   | |   |   \\ | || |   | || (__/ /    | |    \n\r");
	pSocket->Send("	(_____  )|  ___  |   | |   | (\\ \\) || |   | ||  __ (     | |    \n\r");
	pSocket->Send("	      ) || (   ) |   | |   | | \\   || |   | || (  \\ \\    | |    \n\r");
	pSocket->Send("	/\\____) || )   ( |___) (___| )  \\  || (___) || )___) )___) (___ \n\r");
	pSocket->Send("	\\_______)|/     \\|\\_______/|/    )_)(_______)|/ \\___/ \\_______/ \n\r");
	pSocket->Send("                                                      &RKILL OR BE KILLED&w \n\r");
	pSocket->Send("                                         A Game by Ken Mikkelson (2012) \n\r");
	pSocket->Send("                                             Implemented by Owen Varley \n\r");
	pSocket->Send("Press [Any Key] to continue...\n\r");
}

// Display the login menu for Shinobi
void Shinobi::PlayMenu(GameSocket * pSocket)
{
	pSocket->Send("Account for %s\n\r\n\r", pSocket->m_pAccount->m_sName.c_str());

	// Display any Players they have on this server
	CppSQLite3Query query = SqliteDbase::Get()->GetQuery("accountplayers", "accountid", pSocket->m_pAccount->m_nId);

	if (!query.eof())
		pSocket->Send("Characters:\n\r");

	int nCount = 1;
	while (!query.eof())
	{
		int nId = query.getIntField("playerid");
		pSocket->Send(" [%2d] %s\n\r", nCount, SqliteDbase::Get()->GetString("playerdata", "name", "playerid", nId).c_str());
		nCount++;
		query.nextRow();
	}

	if (nCount > 1)
		pSocket->Send("\n\r");

	// Display Menu Options
	pSocket->Send("Continue  - Continue from last game\n\r");
	pSocket->Send("Play <x>  - Enter game with a Character\n\r");
	pSocket->Send("            X can be their name or number\n\r");
	if (SqliteDbase::Get()->GetInt("select count(playerid) from accountplayers where world='Shinobi' and accountid=%d;", pSocket->m_pAccount->m_nId) > 6)
		pSocket->Send("&zCreate    - Already have maximum number of Characters\n\r");
	else
		pSocket->Send("Create    - Create a new Character\n\r");
	pSocket->Send("View <x>  - View Character scoresheet\n\r");
	pSocket->Send("Quit      - Exit Shinobi\n\r\n\r");
	pSocket->Send("What is your selection: ");
	return;
}

// Handle input for Players logged into the Shinobi game
void Shinobi::Interpret(GameSocket * pSocket, std::string sInput)
{
	// First check if they are in a playable state, if not we pass control to the Nanny
	// function which will deal with their input
	if (pSocket->m_nState != SOCKET_PLAYING)
	{
		return Nanny(pSocket, sInput);
	}
	else
	{
		// Playing - Interpret their command normally
		string sCommand;
		string sArgument;

		bool bFound = false;

		// Strip out the first word of the input and interpret this as the command
		// the player wishes to perform, certain commands (those whose keywords are contained
		// within m_PreCommands require that the actual command is NOT stripped from
		// the arguments list
		for (size_t i = 0; i < sInput.length(); i++)
		{
			if (sInput.at(i) == ' ')
			{
				sCommand = sInput.substr(0, i);
				// Skip the first space
				sArgument = sInput.substr(i+1, sInput.size()-i);
				bFound = true;
				break;
			}
		}

		if (!bFound)
			sCommand = sInput;

		// Check if this command is a Preserve command, i.e. we should send the actual input
		// of the command along with the arguments. This is used primarily for the combat system
		// as each move (Dodge, Strike, Feint, etc) is a CmdCombatMove hence we need to know
		// exactly what move they entered
		if (m_PreCommands.find(sCommand) != m_PreCommands.end())
		{
			string sStr = sCommand;
			sStr.append(sArgument);
			sArgument = sStr;
		}

		// First check if they entered a command
		if (!Command(pSocket->m_pPlayer, sCommand, sArgument))
		{
			// If not, was it a move direction?
			if (!pSocket->m_pPlayer->Move(sCommand))
				pSocket->Send("Unknown command: %s.\n\r", sCommand.c_str());
		}

		return;
	}
	return;
}

// Handle player creation in Shinobi
void Shinobi::Nanny(GameSocket * pSocket, string sInput)
{

	// If they are at the SPLASH screen display the Play Menu and enter them into the
	// Play Menu state
	if (pSocket->m_nState == SOCKET_SPLASH)
	{
		pSocket->m_nState = SOCKET_PLAY_MENU;
		PlayMenu(pSocket);
		return;
	}

	// They are at the Play Menu - deal with input appropriately
	if (pSocket->m_nState == SOCKET_PLAY_MENU)
	{
		if (sInput == "create")
		{
			// Check the number of Characters they currently have
			if (SqliteDbase::Get()->GetInt("select count(playerid) from accountplayers where world='Shinobi' and accountid=%d;", pSocket->m_pAccount->m_nId) > 6)
			{
				pSocket->Send("You can only have a maximum of 6 characters! \n\r");
				return;
			}

			pSocket->m_nState = SOCKET_CHARGEN_S_NAME;
			pSocket->Send("Do you wish to have Shinobi generate a name for your Assassin? (Yes/No) ");
			return;
		}
		else if (sInput == "view")
		{
			pSocket->Send("Not Implemented yet.\n\r");
			return;
		}
		else if (sInput == "continue")
		{
			pSocket->Send("Not Implemented yet.\n\r");
			return;
		}
		else if (sInput == "play")
		{
			pSocket->Send("Enter the name of the Character you wish to login with: ");
			return;
		}
		else if (sInput == "quit")
		{
			pSocket->Send("Goodbye! Thanks for playing.\n\r");
			pSocket->Close();
			return;
		}
		else
		{
			if (sInput.length() <= 0)
			{
				pSocket->Send("&RYou must enter a character's name to play!\n\r");
				pSocket->m_nState = SOCKET_PLAY_MENU;
				PlayMenu(pSocket);
				return;
			}
			// Could be entering the name of a character
			int nId = 0;
			if ( (nId = SqliteDbase::Get()->GetInt("playerdata", "playerid", "name", sInput)) > 0)
			{
				// Log them in
				pSocket->Send("The Emperor bids you a fair welcome back to Shinobi, Assassin.\n\r");
				pSocket->m_nState = SOCKET_PLAYING;

				// Create the player object
				Player * pPlayer = new Player();
				// Link their account
				pPlayer->m_pAccount = pSocket->m_pAccount;
				// Get their name based on id
				pPlayer->m_sName = SqliteDbase::Get()->GetString("playerdata", "name", "playerid", nId);
				// Set their Id based on the database unique id
				pPlayer->m_nId = nId;
				// Load the Player
				pPlayer->Load();

				bool bReconnect = false;

				// Check if they are already logged in, if so we will take over that character
				for (map<int, Player*>::iterator it = m_Players.begin(); it != m_Players.end(); it++)
				{
					if ((*it).second->m_nId == pPlayer->m_nId)
					{
						// Give a message to whoever may be connected to the char
						(*it).second->Send("Kicking off old connection!\n\r");
						// Close their socket
						(*it).second->m_pSocket->Close();
						// Delete the socket, we no longer need it
						delete (*it).second->m_pSocket;

						// Delete the player object we started to create above and take over
						// the previous one
						delete pPlayer;
						// Link to the player already logged in
						pPlayer = (*it).second;
						// Link this player to the new socket
						(*it).second->m_pSocket = pSocket;
						// Link our socket to the Player
						pSocket->m_pPlayer = pPlayer;
						bReconnect = true;
						pSocket->Send("Reconnecting!\n\r");
						pSocket->m_pServer->Console("%s has reconnected.", pSocket->m_pPlayer->m_sName.c_str());
					}
				}

				// Create two way link between socket and player
				pPlayer->m_pSocket = pSocket;
				pSocket->m_pPlayer = pPlayer;

				if (!bReconnect)
				{
					// Add the player to the world
					m_Players[pPlayer->m_nId] = pPlayer;
					// Log player into starting room
					pPlayer->m_pRoom = StartingRoom();
					// Send message to players in room
					pPlayer->m_pRoom->Send("%s appears in the room.\n\r", pPlayer->m_sName.c_str());
					// Add the player to the room
					pPlayer->m_pRoom->AddActor(pPlayer);
					// Add the world to the player
					pPlayer->m_pWorld = this;
					// Force them to look
					CmdLook(pPlayer, "");
					pSocket->m_pServer->Console("%s has entered the game.", pSocket->m_pPlayer->m_sName.c_str());
				}
			}
			else
			{
				pSocket->Send("&RInvalid selection.\n\r");
				pSocket->m_nState = SOCKET_PLAY_MENU;
				PlayMenu(pSocket);
				return;
			}
		}
	}
	// Handle Character Generation
	else if (pSocket->m_nState > SOCKET_PLAY_MENU && pSocket->m_nState < SOCKET_PLAYING)
	{
		if (pSocket->m_nState == SOCKET_CHARGEN_S_NAME)
		{
			if (sInput.c_str()[0] == 'y')
			{
				pSocket->Send("\n\rThe engine will now randomly generate a name for your Assassin.\n\r");
				pSocket->Send("Press [Enter] to generate a name and type 'Done' to accept one.\n\r");
				pSocket->Send("If you change your mind and decide upon your own name simply type it in.\n\r");
				pSocket->m_nState = SOCKET_CHARGEN_RANDOM;
			}
			else
			{
				pSocket->Send("Enter the name of your Assassin: ");
				pSocket->m_nState = SOCKET_CHARGEN_NAME;
				return;
			}
		}
		// Random name generator
		if (pSocket->m_nState == SOCKET_CHARGEN_RANDOM)
		{
			string sName;

			if (sInput.empty())
			{
				// Generate random name and output it here
				sName = RandomName();
				bool bExists = true;

				while (bExists)
				{
					sName = RandomName();
					// select
					bExists = SqliteDbase::Get()->RecordExists("playerdata", "playerid", "name", sName);
				}

				pSocket->m_sRandom = sName;
				pSocket->Send(" * %s ", sName.c_str());
			}
			else if (sInput == "done")
			{
				if (pSocket->m_sRandom.empty())
				{
					pSocket->Send("You have not let the system generate one yet! Type [Enter]\n\r");
					return;
				}

				if (SqliteDbase::Get()->RecordExists("playerdata", "playerid", "name", pSocket->m_sRandom))
				{
					pSocket->Send("The Empire already has a Shinobi of that name, select another: ");
					return;
				}
				else
				{
					// Creation of the player will be handled by the CHARGEN_NAME state
					pSocket->m_nState = SOCKET_CHARGEN_NAME;
					sInput = pSocket->m_sRandom;
				}
			}
		}
		if (pSocket->m_nState == SOCKET_CHARGEN_NAME)
		{
			if (sInput.empty())
			{
				pSocket->Send("You have not entered a name!\n\r");
				return;
			}
			// Check the name they entered doesn't exist already
			if (SqliteDbase::Get()->RecordExists("playerdata", "playerid", "name", sInput))
			{
				pSocket->Send("The Empire already has a Shinobi of that name, select another: ");
				return;
			}
			else
			{
				pSocket->Send("May the Divine Emperor bless you on your Quest, %s.\n\r", sInput.c_str());

				// Create the Player and link him into the account
				Player * pPlayer = new Player();
				// Link the account
				pPlayer->m_pAccount = pSocket->m_pAccount;
				// Link the socket
				pPlayer->m_pSocket = pSocket;
				// Link the socket to the player
				pSocket->m_pPlayer = pPlayer;
				// Set the player's name
				pPlayer->m_sName = sInput;
				// Log player into starting room
				pPlayer->m_pRoom = StartingRoom();
				// Send message to players in room
				pPlayer->m_pRoom->Send("%s appears in the room.\n\r", pPlayer->m_sName.c_str());
				// Add the player to the starting room
				pPlayer->m_pRoom->AddActor(pPlayer);
				// Set the player's world
				pPlayer->m_pWorld = this;
				// Force them to look
				CmdLook(pPlayer, "");
				pSocket->m_pServer->Console("[New Player] %s has connected.", pPlayer->m_sName.c_str());

				// Add an entry into the Database
				stringstream sSQL;
				sSQL << "insert into playerdata (name) values ('" << sInput << "');";
				SqliteDbase::Get()->ExecDML(sSQL.str().c_str());

				// Get the ID
				pPlayer->m_nId = SqliteDbase::Get()->GetInt("playerdata", "playerid", "name", pPlayer->m_sName);

				// Initialise Player for this World
				CreatePlayer(pPlayer);

				// Save them!
				pPlayer->Save();

				// Add the player to the player list (has to come after Id is set)
				m_Players[pPlayer->m_nId] = pPlayer;

				// Add a link in the accountplayers
				sSQL.str("");
				sSQL << "insert into accountplayers (accountid, playerid, world) values (" << pPlayer->m_pAccount->m_nId << "," << pPlayer->m_nId << ", '" << this->m_sName << "');";
				SqliteDbase::Get()->ExecDML(sSQL.str().c_str());

				pSocket->m_nState = SOCKET_PLAYING;
				return;
			}
		}
	}
}

// Load Areas
void Shinobi::LoadAreas()
{
	CppSQLite3Query query = SqliteDbase::Get()->GetQuery("areadata", "gameworld", m_sName);

	// Load the areas in
	while (!query.eof())
	{
		Area * pArea = new Area();
		pArea->m_nId = query.getIntField("areaid");
		pArea->m_pWorld = this;
		pArea->Load();
		m_Areas[pArea->m_nId] = pArea;
		query.nextRow();
	}

	Console("%d Areas loaded", m_Areas.size());
	return;
}

// Returning the Starting room
Room * Shinobi::StartingRoom()
{
	Room * pRoom = NULL;
	int nRoom = m_Variables->VarI("starting_room");
	int nArea = m_Variables->VarI("starting_area");

	int nRandArea = Main::rand_range(0, m_Areas.size());
	int nCount = 0;

	for (map<int, Area*>::iterator it = m_Areas.begin(); it != m_Areas.end(); it++)
	{
		// An entry of -1 means select a random area!
		if ((*it).first == nArea || (nArea == -1 && nCount == nRandArea))
		{
			int nRandRoom = Main::rand_range(0, (*it).second->m_Rooms.size());
			int nCount2 = 0;

			for (map<int, Room*>::iterator rt = (*it).second->m_Rooms.begin(); rt != (*it).second->m_Rooms.end(); rt++)
			{
				if ((*rt).first == nRoom || (nRoom == -1 && nRandRoom == nCount2))
				{
					pRoom = (*rt).second;
					break;
				}

				nCount2++;
			}
		}

		nCount ++;
	}

	if (!pRoom)
		Console("Invalid starting room defined");

	return pRoom;
}

// Initialise a New Player Created for this world
void Shinobi::CreatePlayer (Player * pActor)
{
	// Add initial xp to spend
	pActor->m_Variables->Set("curr_xp", 10);
	pActor->m_Variables->Set("total_xp", 10);

	// Add Game Play timer
	pActor->m_Variables->Set("curr_timer", 1);
	pActor->m_Variables->Set("max_timer", 1);

	// Add initial Attributes to the player
	for (map<string,string>::iterator it = m_Attributes.begin(); it != m_Attributes.end(); it++)
	{
		string sCurrent = "curr_";
		string sMax 	= "max_";
		sCurrent.append((*it).first);	// curr_attr_1
		sMax.append((*it).first);		// max_attr_1
		string sAttr = (*it).second;
		pActor->m_Variables->Set(sCurrent, 1);
		pActor->m_Variables->Set(sMax, 1);
	}
}

// Delete a player, removing them from the World
void Shinobi::DeletePlayer (Player * pPlayer)
{
	// TODO Archive their details
	// Remove all their database entries
	SqliteDbase::Get()->DeleteEntry("playerdata", "playerid", pPlayer->m_nId);
	SqliteDbase::Get()->DeleteEntry("playervariables", "playerid", pPlayer->m_nId);
	SqliteDbase::Get()->DeleteEntry("accountplayers", "playerid", pPlayer->m_nId);
	// Remove them from the Room
	((Actor*)pPlayer)->m_pRoom->RemActor(pPlayer);
	// Remove them from the Game
	RemPlayer(pPlayer);
	// Return the player to the player menu
	pPlayer->m_pSocket->m_nState = SOCKET_PLAY_MENU;
	PlayMenu(pPlayer->m_pSocket);
	return;
}


// Message handling

// Send - Send a message to all players in the GameWorld
void Shinobi::Send(const std::string &sMsg, ...)
{
    char buf[MAX_STRING_LENGTH*2];
    va_list args;

    va_start(args, sMsg);
    vsprintf(buf, sMsg.c_str(), args);
    va_end(args);

    string sEdit(buf);

    GameWorld::Send(sEdit);

    return;
}

// Console - Display a message on the console
void Shinobi::Console(const std::string &sMsg, ...)
{
    char buf[MAX_STRING_LENGTH*2];
    va_list args;

    va_start(args, sMsg);
    vsprintf(buf, sMsg.c_str(), args);
    va_end(args);

    string sEdit(buf);

    std::cout << GameServer::StrTime() << "[Shinobi] " << sEdit << endl;
    return;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
// ACHILLARY FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////////////////

// Returns the string representation of a numerical health value
string Shinobi::Health (int nCurr, int nMax)
{
	float fPercentage = (float)nCurr/(float)nMax * 100.0;

	if (fPercentage == 100.0)
		return "unhurt";
	else if (fPercentage >= 90.0)
		return "scratched";
	else if (fPercentage >= 66.6)
		return "bruised";
	else if (fPercentage >= 33.3)
		return "wounded";
	else
		return "severely injured";
}

// Returns the string representation for an Attribute
string Shinobi::AttrName(int nId)
{
	stringstream sAtt;
	sAtt << "attr_" << nId;
	string sAttr = sAtt.str();

	for (map<string,string>::iterator it = m_Attributes.begin(); it != m_Attributes.end(); it++)
	{
		if (StrEquals((*it).first, sAttr))
		{
			return (*it).second;
		}
	}

	return "";
}

// Returns the string representation for an Affect
string Shinobi::AffectName (int nAffect)
{
	if (nAffect == AFF_ATTR_1)
		return AttrName(1);
	else if (nAffect == AFF_ATTR_2)
		return AttrName(2);
	else if (nAffect == AFF_ATTR_3)
		return AttrName(3);
	else if (nAffect == AFF_ATTR_4)
		return AttrName(4);
	else if (nAffect == AFF_ATTR_5)
		return AttrName(5);

	return "none";
}

// Combat Manager
// Handles combat between two playrs

void Shinobi::CombatManager()
{
	list<vector<Combat*>::iterator> DeleteList;

	for (vector<Combat*>::iterator it = m_Combat.begin(); it != m_Combat.end(); it++)
	{
		(*it)->Update();

		if ((*it)->m_bEnded)
			DeleteList.push_back(it);
	}

	// Delete any combat instances that have now expired, this will also now free the memory of
	// either player in the case that they were killed
	for (list<vector<Combat*>::iterator>::iterator it = DeleteList.begin(); it != DeleteList.end(); it++)
	{
		Actor * pAttacker = (*(*it))->m_pAttacker;
		Actor * pDefender = (*(*it))->m_pDefender;

		if (pAttacker)
		{
			pAttacker->m_pCombat = NULL;

			if (pAttacker->HasFlag("delete"))
			{
				RemPlayer((Player*)pAttacker);
				delete pAttacker;
			}
		}

		if (pDefender)
		{
			pDefender->m_pCombat = NULL;

			if (pDefender->HasFlag("delete"))
			{
				DeletePlayer((Player*)pDefender);
				delete pDefender;
			}
		}

		m_Combat.erase(*it);
		delete (*(*it));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Command handling
////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Shinobi::Command(Actor * pActor, string sCommand, string sArguments)
{
	map<string, InputCmd>::iterator iter = m_Commands.find(sCommand);

	if (iter != m_Commands.end())
	{
		(this->*iter->second) (pActor, sArguments);
		return true;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// GLOBAL COMMANDS
/////////////////////////////////////////////////////////////////////////////////////////////////////

// Command - Who
// Displays a list of the current assassins playing Shinobi, also displays some rudimentary scores
// and perhaps rank the who list by lethalist player.
bool Shinobi::CmdWho(Actor * pActor, string sArguments)
{
	pActor->Send("%s - %s\n\r", pActor->m_pWorld->m_sName.c_str(), pActor->m_pWorld->m_sDesc.c_str());

	for (map<int, Player*>::iterator it = m_Players.begin(); it != m_Players.end(); it++)
	{
		pActor->Send("  * %s%s %s\n\r", (*it).second->m_pSocket->m_Variables["afk"] == "yes" ? "[AFK] " : "",
									   (*it).second->m_sName.c_str(),
									   (*it).second->m_Variables->Var("title").empty() ? "" : (*it).second->m_Variables->Var("title").c_str());
	}
	pActor->Send("\n\rTotal Players: %d\n\r", m_Players.size());
	return true;
}

// Command - Whois
// Displays Description and Kill Rating of a player, Global command
bool Shinobi::CmdWhois(Actor * pActor, string sArguments)
{
	for (map<int, Player*>::iterator it = m_Players.begin(); it != m_Players.end(); it++)
	{
		if (StrContains((*it).second->m_sName, sArguments))
		{
			pActor->Send("Whois:\n\r");
			pActor->Send("Description: %s\n\r", (*it).second->m_Variables->Var("description").c_str());
			pActor->Send("Kill Rating: %s\n\r", (*it).second->m_Variables->Var("kill_rating").c_str());
			return true;
		}
	}

	pActor->Send("No such player online. Try using who first.\n\r");
	return true;
}

// Command - Time
// Displays the current system time
bool Shinobi::CmdTime (Actor * pActor, string sArguments)
{
	time_t rawtime = GameServer::Time();
	struct tm * timeinfo;
	struct tm * gmt;
	gmt = gmtime ( &rawtime );
	timeinfo = localtime ( &rawtime );

	if (sArguments == "rand")
	{
		pActor->Send("Rand: D100(%d) D6(%d)\n\r", Main::rand_range(1, 100), Main::rand_range(1, 6));
		return true;
	}

	pActor->Send("World times:\n\r");
	pActor->Send("London (GMT): %2d:%02d\n\r", gmt->tm_hour%24, gmt->tm_min);
	pActor->Send("\n\rThe current system time is: %s", asctime (timeinfo));



	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// COMMUNICATION COMMANDS
/////////////////////////////////////////////////////////////////////////////////////////////////////

// Command - Chat
// Sends a global Out Of Character (OOC) message
bool Shinobi::CmdChat(Actor * pActor, string sArguments)
{
	for (map<int, Player*>::iterator it = m_Players.begin(); it != m_Players.end(); it++)
	{
		if (pActor == (*it).second)
			continue;

		(*it).second->Send("%s: %s\n\r", pActor->m_sName.c_str(), sArguments.c_str());

	}

	pActor->Send("You: %s\n\r", sArguments.c_str());
	return true;
}

// Command - Say
// Sends a message from a player to all players in the room
bool Shinobi::CmdSay(Actor * pActor, string sArguments)
{
	if (!pActor->m_pRoom)
		return true;

	if (pActor->IsAffected("meditation"))
		pActor->RemAffect("meditation");

	pActor->Send("You say, '%s'\n\r", sArguments.c_str());

	// Send the message to the room
	for (list<Actor*>::iterator it = pActor->m_pRoom->m_Actors.begin(); it != pActor->m_pRoom->m_Actors.end(); it++)
	{
		if ((*it) == pActor)
			continue;

		if ((*it)->Attribute("attr_3") > pActor->m_Variables->VarI("hide_value"))
			(*it)->Send("%s says, '%s'\n\r", ToCapitals(pActor->m_sName).c_str(), sArguments.c_str());
		else
			(*it)->Send("An unidentified voice says, '%s'\n\r", sArguments.c_str());
	}
	return true;
}

// Command - OSay
// Sends a message from a player to all players in the room Out of Character
bool Shinobi::CmdOsay(Actor * pActor, string sArguments)
{
	if (!pActor->m_pRoom)
		return true;

	pActor->Send("You say OOCly, '%s'\n\r", sArguments.c_str());

	// Send the message to the room
	// Send the message to the room
	for (list<Actor*>::iterator it = pActor->m_pRoom->m_Actors.begin(); it != pActor->m_pRoom->m_Actors.end(); it++)
	{
		if ((*it) == pActor)
			continue;

		if ((*it)->Attribute("attr_3") > pActor->Attribute("hide_value"))
			(*it)->Send("%s says OOCly, '%s'\n\r", ToCapitals(pActor->m_sName).c_str(), sArguments.c_str());
		else
			(*it)->Send("An unidentified voice says OOCly, '%s'\n\r", sArguments.c_str());
	}

	return true;
}

// Command - Shout
// Sends a message from a player to all players in the current room and also to any room that is
// linked to this room by an exit
bool Shinobi::CmdShout(Actor * pActor, string sArguments)
{
	list<Room*> RoomList;

	if (!pActor->m_pRoom)
		return true;

	RoomList.push_back(pActor->m_pRoom);

	pActor->Send("You shout, '%s'\n\r", sArguments.c_str());

	// Add all rooms that have an exit from this room
	for (map<string, Exit*>::iterator it = pActor->m_pRoom->m_Exits.begin(); it != pActor->m_pRoom->m_Exits.end(); it++)
	{
		Room * pNewRoom = m_Areas[pActor->m_pRoom->m_pArea->m_nId]->m_Rooms[(*it).second->m_nTo];

		if (pNewRoom)
			RoomList.push_back(pNewRoom);
	}

	// Send the message to the room
	for (list<Room*>::iterator it = RoomList.begin(); it != RoomList.end(); it++)
		(*it)->Send(pActor, "%s shouts, '%s'\n\r", ToCapitals(pActor->m_sName).c_str(), sArguments.c_str());

	return true;
}

// Command - Emote
// Sends a message from a player to all players in the room
bool Shinobi::CmdEmote(Actor * pActor, string sArguments)
{
	if (!pActor->m_pRoom)
		return true;

	if (pActor->IsAffected("meditation"))
		pActor->RemAffect("meditation");

	// Send the message to the room
	pActor->m_pRoom->Send("%s %s\n\r", ToCapitals(pActor->m_sName).c_str(), sArguments.c_str());
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// MOVEMENT AND WORLD COMMANDS
/////////////////////////////////////////////////////////////////////////////////////////////////////

// Command - Look
// Perception of other items, rooms and players in the game
bool Shinobi::CmdLook(Actor * pActor, string sArguments)
{
	if (pActor->IsAffected("meditation"))
		pActor->RemAffect("meditation");

	// Show room contents if they don't provide an argument
	if (sArguments.empty())
	{
		// Room Name
		// This is the room's description, isn't it great?
		//
		// This is an item, So is this, this is one as well, guess what this is,
		// There are 10 of this one(10), only two of this (2)
		// This is a NPC
		// So is this
		//
		// Exits:
		// West - Another room

		if (pActor->m_pRoom)
		{
			// Name
			pActor->Send("\n\r%s\n\r", pActor->m_pRoom->m_sName.c_str());

			// Description
			pActor->Send("%s\n\r\n\r", pActor->m_pRoom->m_sDesc.c_str());

			// Items
			// Updated (18/9/12) to Handle multiple items
			map<string, int> ItemMap;

			for (list<Item*>::iterator it = pActor->m_pRoom->m_Items.begin(); it != pActor->m_pRoom->m_Items.end(); it ++)
			{
				ItemMap[(*it)->m_sName] += 1;
			}

			bool bPrevious = false;
			bool bFound = false;
			for (map<string, int>::iterator it = ItemMap.begin(); it != ItemMap.end(); it ++)
			{
				bFound = true;
				if ( (*it).second > 1)
					pActor->Send("%s[%d] %s", bPrevious ? ", " : "", (*it).second, (*it).first.c_str());
				else
					pActor->Send("%s%s", bPrevious ? ", " : "", (*it).first.c_str());
				bPrevious = true;
			}
			if (bFound)
				pActor->Send("\n\r");


			// Actors
			for (list<Actor*>::iterator it = pActor->m_pRoom->m_Actors.begin(); it != pActor->m_pRoom->m_Actors.end(); it++)
			{
				if ((*it) == pActor)
					continue;

				bFound = true;

				// Check for hiding characters
				if ((*it)->IsAffected("hide"))
				{
					// If the viewer's wisdom is higher than the shroud value, they see them
					if (pActor->Attribute("attr_3") > (*it)->m_Variables->VarI("hide_value"))
						pActor->Send("%s", (*it)->RoomDesc(pActor, true).c_str());
				}
				else
					pActor->Send("%s\n\r", (*it)->RoomDesc(pActor, false).c_str());
			}

			if (bFound)
				pActor->Send("\n\r");

			pActor->Send("Exits:\n\r");
			for (std::map<string, Exit*>::iterator it = pActor->m_pRoom->m_Exits.begin(); it != pActor->m_pRoom->m_Exits.end(); it ++)
			{
				if (!(*it).second)
					continue;

				pActor->Send("%s - %s\n\r", ToCapital((*it).second->m_sDir).c_str(), ToCapital(pActor->m_pRoom->m_pArea->m_Rooms[(*it).second->m_nTo]->m_sName).c_str());
			}
			pActor->Send("\n\r");
		}
	}
	else
	{
		// Try to find their target

		// [1] Check for an actor in the room
		for (list<Actor*>::iterator it = pActor->m_pRoom->m_Actors.begin(); it != pActor->m_pRoom->m_Actors.end(); it++)
		{
			if ((*it) == pActor)
				continue;

			if (StrContains((*it)->m_sName, sArguments))
			{
				// Show description and equipment
				pActor->Send("Looking at %s you see:\n\r", ToCapital((*it)->m_sName).c_str());
				// Show warrior rating for NPCs
				if ((*it)->m_nType == _NPC)
				{
					if ((*it)->m_Variables->VarI("warrior_rating") == 1)
						pActor->Send("An adept warrior.\n\r");
					if ((*it)->m_Variables->VarI("warrior_rating") == 2)
						pActor->Send("A skilled warrior.\n\r");
					if ((*it)->m_Variables->VarI("warrior_rating") == 3)
						pActor->Send("A lethal warrior.\n\r");
				}

				if (!(*it)->m_Variables->Var("desc").empty())
					pActor->Send("%s\n\r", (*it)->m_Variables->Var("desc").c_str());
				else
					pActor->Send(" Nothing out of the ordinary.\n\r");

				// Show health
				if ((*it)->m_nType == _PLAYER)
					pActor->Send("Health: %s\n\r", ToCapital(Health((*it)->m_Variables->VarI("curr_hp"), (*it)->m_Variables->VarI("max_hp"))).c_str());


				// If they look at an NPC show what that NPC will drop on death
				if ((*it)->m_nType == _NPC)
				{
					for (map<int,int>::iterator jt = (*it)->m_Loot.begin(); jt != (*it)->m_Loot.end(); jt++)
					{
						pActor->Send(" Item: %d (%d%%)\n\r", (*jt).first, (*jt).second);
					}
				}

				// If they are a Player, show any equipped items
				if ((*it)->m_nType == _PLAYER)
				{
					pActor->Send("Equipped:\n\r");

					for (map<string, Item*>::iterator jt = (*it)->m_Equipment.begin(); jt != (*it)->m_Equipment.end(); jt++)
					{
						string sWear = "<";
						sWear.append((*jt).first);
						sWear.push_back('>');
						pActor->Send(" %-10s %s%s\n\r", sWear.c_str(), (*jt).second->HasFlag("broken") ? "(Broken)" : "", (*jt).second->m_sName.c_str());
					}
				}

				// Compare examiner's cunning vs examinee's wisdom to determine whether a message is required
				if ((*it)->m_nType == _PLAYER)
				{
					if (pActor->Attribute("attr_4") <= (*it)->Attribute("attr_3"))
					{
						(*it)->Send("%s examines you closely.\n\r", ToCapitals(pActor->m_sName).c_str());

						// Non stealthy target, remove an ambush state if they had one previously
						if (pActor->m_Variables->HasVar("combat_ambush"))
							pActor->m_Variables->RemVar("combat_ambush");
					}
					else
					{
						// Actor performs a stealth look/target this has to be noted for the initial combat round
						pActor->m_Variables->Set("combat_ambush", 1);
					}
				}

				pActor->m_pTarget = (*it);

				return true;
			}
		}

		pActor->Send("&RYou do not see that here.\n\r");
		pActor->Send("&rSyntax: look/target <Actor>\n\r");
		pActor->Send("&r        examine <Item>\n\r");
		return true;
	}

	return true;
}

bool Shinobi::CmdQuit(Actor * pActor, string sArguments)
{
	pActor->Send("Farewell Assassin, until your return.\n\r");


	// Return the player to the player menu
	Player * pPlayer = (Player*) pActor;
	pPlayer->m_pSocket->m_nState = SOCKET_PLAY_MENU;
	PlayMenu(pPlayer->m_pSocket);

	// Remove the Player from the room
	if (pPlayer->m_pRoom)
		pPlayer->m_pRoom->RemActor(pActor);

	// Send message to players in room
	pPlayer->m_pRoom->Send("%s disappears from the room.\n\r", pPlayer->m_sName.c_str());

	// Remove the Player from the world
	pPlayer->m_pWorld->RemPlayer(pPlayer);

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// CHARACTER COMMANDS
/////////////////////////////////////////////////////////////////////////////////////////////////////

// Command - Title
// Set a player's title as seen on who
bool Shinobi::CmdTitle(Actor * pActor, string sArguments)
{
	// Check for some input first
	if (sArguments.empty())
	{
		pActor->Send("&RYou must enter something to set your title to.\n\r");
		pActor->Send("&rSyntax: title <new title>\n\r");
		pActor->Send("See help title\n\r");
		return true;
	}

	// Check length
	if (sArguments.length() > (unsigned int)pActor->m_pWorld->m_Variables->VarI("title_length"))
	{
		pActor->Send("&RThe maximum length of a title is %d characters, your entry was %d.\n\r", pActor->m_pWorld->m_Variables->VarI("title_length"), sArguments.length());
		return true;
	}

	pActor->m_Variables->Set("title", sArguments);
	pActor->Send("&GNew title set.\n\r");
	((Player*)pActor)->Save("title", sArguments);
	return true;
}

// Command - Desc
// Set a player's description shown when another player types look <target>
bool Shinobi::CmdDesc(Actor * pActor, string sArguments)
{
	// Check for some input first
	if (sArguments.empty())
	{
		pActor->Send("&RYou must enter something to set your description to.\n\r");
		pActor->Send("&rSyntax: desc <new description>\n\r");
		pActor->Send("See help desc\n\r");
		return true;
	}

	// Check length
//	if (sArguments.length() > 40)
//	{
//		pActor->Send("&RThe maximum length of a title is 40 characters, your entry was %d.\n\r", sArguments.length());
//		return true;
//	}

	pActor->m_Variables->Set("desc", sArguments);
	pActor->Send("&GNew description set.\n\r");
	// Save this
	((Player*)pActor)->Save("desc", sArguments);
	return true;
}

// Command - Score
// Display character's score sheet
bool Shinobi::CmdScore(Actor * pActor, string sArguments)
{
	// Scoresheet AL1
	// Name:
	// Unspent XP  / Total XP
	// Total time spent playing: ie, age.
	// Kills, mob kills, kills, and deaths
	// Current health:  Given as an Adjective. (Bruised, Hurt, Wounded, Severely Injured, Incapacitated)
	//
	// 		Strength
	// 		Agility
	// 		Wisdom
	// 		Cunning
	// 		Spirit	Both permanent and unspent Spirit Energy
	//
	// Achievements
	//
	// Affected by
	// 		Disguises
	// 		Diseases or Poisons
	// 		Not sneaking -- sneaking state
	// 		Combat conditions (Prone, Dazed, Cornered)
	// 		Trappings:  eg, +1 Strength
	//

	pActor->Send("Name: %s\n\r", ToCapitals(pActor->m_sName).c_str());
	pActor->Send("Unspent XP (%d) / Total XP (%d)\n\r", pActor->m_Variables->VarI("curr_xp"), pActor->m_Variables->VarI("total_xp"));
	pActor->Send("Total time spent playing: %0.2f\n\r", float(pActor->m_Variables->VarI("max_timer")) / 360.0);
	pActor->Send("Kills (%d), NPC Kills (%d), Deaths (%d)\n\r", pActor->m_Variables->VarI("kills"), pActor->m_Variables->VarI("npc_kills"), pActor->m_Variables->VarI("deaths"));
	if (pActor->m_nType == _PLAYER)
	{
		pActor->Send("Flags: ");
		// Show Flags for the moment
		for (map<string, int>::iterator it = pActor->m_Flags.begin(); it != pActor->m_Flags.end(); it++)
			pActor->Send("%s ", (*it).first.c_str());

		pActor->Send("\n\r");
	}
	pActor->Send("Current Health: %s\n\r", ToCapital(Health(pActor->m_Variables->VarI("curr_hp"), pActor->m_Variables->VarI("max_hp"))).c_str());	// TODO
	pActor->Send("\n\r");

	for (map<string,string>::iterator it = pActor->m_pWorld->m_Attributes.begin(); it != pActor->m_pWorld->m_Attributes.end(); it++)
	{
		string sCurrent = "curr_";
		string sMax 	= "max_";
		sCurrent.append((*it).first);	// curr_attr_1
		sMax.append((*it).first);		// max_attr_1
		string sAttr = (*it).second;

		int nCurr = pActor->Attribute((*it).first);
		int nMax = pActor->AttrMax((*it).first);

		pActor->Send("    %-10s %s%d\n\r", sAttr.c_str(),
										 nCurr > nMax ? "&G" : nCurr < nMax ? "&R" : "",
										 nCurr  );

	}

	// TODO Achievements

	// Affects
	pActor->Send("\n\r");
	CmdAffects(pActor, "");

	// TODO Finish
	pActor->Send("\n\r");
	return true;
}

// Command - Save
// Save this character's details
bool Shinobi::CmdSave(Actor * pActor, string sArguments)
{
	pActor->Send("&GYour player character is saved automatically here.\n\r");
//	((Player*)pActor)->Save();
//	pActor->Send("&GPlayer saved.\n\r");
	return true;
}

// Command - Train
// Use experience to improve your character, the cost to train is equal to
// the current attribute rating e.g. to raise Spirit from 2 to 3 costs 2 XP
bool Shinobi::CmdTrain(Actor * pActor, string sArguments)
{
	int nXP = pActor->m_Variables->VarI("curr_xp");

	if (nXP <= 0)
	{
		pActor->Send("You do not have any available experience points (XP) to spend.\n\r");
		pActor->Send("Hint: Gain XP by killing other players.\n\r");
		return true;
	}
	// With no arguments we list any attributes they have the XP to raise along with syntax
	if (sArguments.empty())
	{
		stringstream sAttributes;

		for (map<string,string>::iterator it = pActor->m_pWorld->m_Attributes.begin(); it != pActor->m_pWorld->m_Attributes.end(); it++)
		{
			string sCurrent = "curr_";
			string sMax 	= "max_";
			sCurrent.append((*it).first);	// curr_attr_1
			sMax.append((*it).first);		// max_attr_1
			string sAttr = (*it).second;
			int nCurr = pActor->m_Variables->VarI(sCurrent);

			if (nCurr <= nXP)
				sAttributes << ToCapital(sAttr).c_str() << " ";
		}

		if (sAttributes.str().length() > 0)
		{
			pActor->Send("You have %d XP. Enough to increase the following attributes:\n\r", nXP);
			pActor->Send("%s\n\r", sAttributes.str().c_str());
			pActor->Send("Syntax: train <attribute name>\n\r");
		}
		else
		{
			pActor->Send("You do not have enough experience points to increase any of your attributes.\n\r");
			pActor->Send("Hint: It costs the same amount of XP as your attribute to increase one.\n\r");
			pActor->Send("    : E.g. To raise Spirit from 2 to 3 costs 2 XP.\n\r");
		}
	}
	else
	{
		// They entered an attribute to raise, do they have the required XP? Is it a valid attribute?
		for (map<string,string>::iterator it = pActor->m_pWorld->m_Attributes.begin(); it != pActor->m_pWorld->m_Attributes.end(); it++)
		{
			string sCurrent = "curr_";
			string sMax 	= "max_";
			sCurrent.append((*it).first);	// curr_attr_1
			sMax.append((*it).first);		// max_attr_1
			string sAttr = (*it).second;
			int nCurr = pActor->m_Variables->VarI(sCurrent);

			// Did we find a match?
			if (StrEquals(sArguments, (*it).second))
			{
				// Its a match, do they have the required XP?
				if (nXP >= nCurr)
				{
					// They have the XP, will this increase it above the max?
					if ((nCurr+1) >= m_Variables->VarI("attribute_max"))
					{
						// Too high
						pActor->Send("&RYou cannot increase that attribute any further.\n\r");
						return true;
					}
					else
					{
						// Increase the attribute
						pActor->m_Variables->AddToVar(sCurrent, 1);
						// Increase the maximum as well
						pActor->m_Variables->AddToVar(sMax, 1);
						// Remove the spent xp
						pActor->m_Variables->AddToVar("curr_xp", -nCurr);
						((Player*)pActor)->Save(sMax, nCurr+1);
						((Player*)pActor)->Save(sCurrent, nCurr+1);
						((Player*)pActor)->Save("curr_xp", nXP-nCurr);
						pActor->Send("&GYou invest your XP in improving your %s.\n\r", sAttr.c_str());
						return true;
					}
				}
				else
				{
					pActor->Send("&RIt would cost %d XP to raise that stat. You do not have enough!\n\r", nCurr);
					return true;
				}
				return true;
			}
		}

		pActor->Send("&RThat is not a valid attribute to increase, options are:\n\r");
		for (map<string,string>::iterator it = pActor->m_pWorld->m_Attributes.begin(); it != pActor->m_pWorld->m_Attributes.end(); it++)
			pActor->Send("%s ", (*it).second.c_str());
		pActor->Send("\n\r&rSyntax: train <attribute name>\n\r");
	}

	return true;
}

// Command - Affects
// Displays the affects currently affecting a character
bool Shinobi::CmdAffects(Actor * pActor, string sArguments)
{
	pActor->Send("Current Affects:\n\r");
	if (pActor->m_Affects.size() <= 0)
	{
		pActor->Send("    Nothing is affecting your character at present.\n\r");
		return true;
	}

	for (map <string, Affect*>::iterator it = pActor->m_Affects.begin(); it != pActor->m_Affects.end(); it++)
	{
		Affect * pAffect = (*it).second;
		int nRaw = pAffect->m_nSeconds;
		string sAffects = "";

		// Display affects details
		if (pAffect->m_nLocation != AFF_NONE)
			sAffects = AffectName(pAffect->m_nLocation);

		// Sneak
		// Hide
		// Frenzy - Remaining 2 mins 33 seconds

		if (nRaw > 60)
			pActor->Send("    %s  [Remaining: %d mins %d seconds]\n\r", ToCapital((*it).first).c_str(), nRaw/60, nRaw%60);
		else if (nRaw > 0)
			pActor->Send("    %s  [Remaining: %d seconds]\n\r", ToCapital((*it).first).c_str(), nRaw);
		else
			pActor->Send("    %s\n\r", ToCapital((*it).first).c_str());

		// Display location details
		if (pAffect->m_sName == "sneak")
			pActor->Send("     - Silently move from room to room.\n\r");
		else if (pAffect->m_sName == "hide")
			pActor->Send("     - Remain hidden from sight whilst stationary. Shroud rating %d\n\r", pActor->m_Variables->VarI("hide_value"));
		else if (pAffect->m_sName == "meditation")
			pActor->Send("     - Regain %d point of Spirit and %d health every 6 seconds\n\r", pActor->m_pRoom->HasFlag("holy") ? 2 : 1, pActor->m_pRoom->HasFlag("holy") ? (pActor->Attribute("attr_1")+6)*2 : (pActor->Attribute("attr_1")+6));
		else if (pAffect->m_sName == "thrown")
			pActor->Send("     - Unable to throw another item for %d seconds\n\r", pAffect->m_nSeconds);
		else if (pAffect->m_sName == "cornered")
			pActor->Send("     - Unable to dodge next round in Combat\n\r");
		else if (pAffect->m_sName == "overextended")
			pActor->Send("     - Unable to strike next round in Combat\n\r");
		else if (pAffect->m_sName == "bleeding" || pAffect->m_sName == "bleeding heavily")
			pActor->Send("     - Badly hurt, you will leave a blood trail\n\r");
		else if (pAffect->m_sName == "prone")
			pActor->Send("     - Suffer a 50%% penalty to strike, surge to remove\n\r");
		else if (pAffect->m_sName == "dazed")
			pActor->Send("     - Suffer a 50%% penalty to Wisdom, surge to remove\n\r");
		else if (pAffect->m_sName == "staggered")
			pActor->Send("     - Suffer a 50%% penalty to Agility, surge to remove\n\r");
		else if (pAffect->m_nLocation != AFF_NONE)
			pActor->Send("     - Modifies %s by %d.\n\r", sAffects.c_str(), pAffect->m_nModifier);
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
// ITEMS AND INTERATION COMMANDS
/////////////////////////////////////////////////////////////////////////////////////////////////////

// Command - Get
// Removes an item from the ground and places it into a character's inventory
// Syntax: Get <item>   				-- For items on the ground
//         Get <item> from <item>		-- For items in containers
bool Shinobi::CmdGet(Actor * pActor, string sArguments)
{
	// Check they entered an argument
	if (sArguments.empty())
	{
		pActor->Send("&RWhat do you wish to get?\n\r");
		pActor->Send("&rSyntax: get <item name>\n\r");
		pActor->Send("&r        get <item name> from <item>\n\r");
		return true;
	}

	Item * pItem = NULL;
	Item * pContainer = NULL;

	// Are they trying to get an item from another item?
	if (StrContains(sArguments, " from "))
	{
		// Split the sArguments string in half using the 'from' string
		size_t pos = sArguments.find(" from ");
		string sContainer = sArguments.substr(pos+6, sArguments.length());
		string sItem = sArguments.substr(0, pos);

		// Try to find the container in the room first
		pContainer = FindItem(sContainer, pActor->m_pRoom);

		if (!pContainer)
			pContainer = FindItem(sContainer, pActor);

		if (!pContainer)
		{
			pActor->Send("&RYou can't seem to find %s anywhere.\n\r", sContainer.c_str());
			pActor->Send("&rSyntax: get <item> from <container>\n\r");
			return true;
		}

		pItem = FindItem(sItem, pContainer);
	}
	else
	{
		pItem = FindItem(sArguments, pActor->m_pRoom);
	}

	if (pItem)
	{
		if (pActor->IsAffected("hide"))
		{
			pActor->RemAffect("hide");
			pActor->m_Variables->RemVar("hide_value");
			pActor->Send("Your action causes you to step from the shadows!\n\r");
		}

		if (pItem->HasFlag("No Get"))
		{
			pActor->Send("&RYou cannot pick up that item.\n\r");
			return true;
		}

		if (pActor->IsAffected("meditation"))
			pActor->RemAffect("meditation");

		if (pContainer)
		{
			pContainer->m_Contents.remove(pItem);
			pActor->m_Inventory.push_back(pItem);
			pActor->Send("You get %s from %s.\n\r", pItem->m_sName.c_str(), pContainer->m_sName.c_str());
			pActor->m_pRoom->Send(pActor, "%s gets %s from %s.\n\r", pActor->m_sName.c_str(), pItem->m_sName.c_str(), pContainer->m_sName.c_str());
		}
		else
		{
			pActor->m_pRoom->m_Items.remove(pItem);
			pActor->m_Inventory.push_back(pItem);
			pActor->Send("You pick up %s.\n\r", pItem->m_sName.c_str());
			pActor->m_pRoom->Send(pActor, "%s picks up %s.\n\r", pActor->m_sName.c_str(), pItem->m_sName.c_str());
		}
		return true;
	}
	else
	{
		if (!pContainer)
		{
			pActor->Send("&RYou do not see that item here!\n\r");
			pActor->Send("&rType look to view a list of items here.\n\r");
			pActor->Send("&rSyntax: get <item name>\n\r");
		}
		else
		{
			pActor->Send("&RYou do not see that item within %s.\n\r", pContainer->m_sName.c_str());
			pActor->Send("&rType examine <item> to view the contents of an item.\n\r");
			pActor->Send("&rSyntax: get <item name> from <item>\n\r");

		}

		return true;
	}
}

bool Shinobi::CmdDrop(Actor * pActor, string sArguments)
{
	// Check they entered an argument
	if (sArguments.empty())
	{
		pActor->Send("&RWhat do you wish to drop?\n\r");
		pActor->Send("&rType inventory to view what you are carrying currently.\n\r");
		pActor->Send("&rSyntax: drop <item name>\n\r");
		return true;
	}

	Item * pItem = FindItem(sArguments, pActor);

	if (pItem)
	{
		if (pActor->IsAffected("hide"))
		{
			pActor->RemAffect("hide");
			pActor->m_Variables->RemVar("hide_value");
			pActor->Send("Your action causes you to step from the shadows!\n\r");
		}

		if (pActor->IsAffected("meditation"))
			pActor->RemAffect("meditation");

		pActor->m_Inventory.remove(pItem);
		pActor->m_pRoom->m_Items.push_back((pItem));
		pActor->Send("You drop %s.\n\r", pItem->m_sName.c_str());
		pActor->m_pRoom->Send(pActor, "%s drops %s.\n\r", pActor->m_sName.c_str(), pItem->m_sName.c_str());
		return true;
	}
	else
	{
		pActor->Send("&RYou don't seem to be carrying that item.\n\r");
		pActor->Send("&rType inventory to view what you are carrying currently.\n\r");
		pActor->Send("&rSyntax: drop <item name>\n\r");
	}
	return true;
}

bool Shinobi::CmdWear(Actor * pActor, string sArguments)
{
	// Check they entered an argument
	if (sArguments.empty())
	{
		pActor->Send("&RWhat do you wish to wear?\n\r");
		pActor->Send("&rType inventory to view what you are carrying currently.\n\r");
		pActor->Send("&rSyntax: wear <item name>\n\r");
		return true;
	}

	for (list<Item*>::iterator it = pActor->m_Inventory.begin(); it != pActor->m_Inventory.end(); it++)
	{
		if (StrContains((*it)->m_sName, sArguments))
		{
			if (pActor->IsAffected("hide"))
			{
				pActor->RemAffect("hide");
				pActor->m_Variables->RemVar("hide_value");
				pActor->Send("Your action causes you to step from the shadows!\n\r");
			}

			if (pActor->IsAffected("meditation"))
				pActor->RemAffect("meditation");

			// Check the item has a valid wear location
			if ((*it)->m_Variables->Var("wearloc").empty())
			{
				pActor->Send("&RThat item cannot be worn.\n\r");
				return true;
			}

			// Remove any items already in that location
			string sWearloc = (*it)->m_Variables->Var("wearloc");
			Item * pWorn = pActor->GetEquipped(sWearloc);

			bool bMessage = true;

			// We don't show a message if the item is a disguise
			if ((*it)->m_Variables->Var("type") == "disguise")
			{
				bMessage = false;

				// Add the disguise effect
				if (!pActor->IsAffected("disguise"))
				{
					pActor->AddAffect("disguise", AFF_ATTR_4, (*it)->m_Variables->VarI("curr_quality"), -1, -1);
					pActor->Send("You don your disguise.\n\r");
				}

			}

			if (pWorn)
			{
				pActor->m_Inventory.push_back(pWorn);
				pActor->m_Equipment.erase(sWearloc);

				if (bMessage)
					pActor->Act("", "", NULL, pWorn, (*it), "$n remove$p $o and replace$p it with $O.");
			}
			else
			{
				if (bMessage)
				{
					if (sWearloc == "hands")
						pActor->Act("", "", NULL, (*it), NULL, "$n wield$p $o.");
					else
						pActor->Act("", "", NULL, (*it), NULL, "$n wear$p $o.");
				}
			}

			Item * pItem = (*it);
			pActor->m_Inventory.remove(*it);
			pActor->m_Equipment[pItem->m_Variables->Var("wearloc")] = pItem;

			// Sneak check
			if (pActor->IsAffected("sneak") && pActor->GetEquipped("body"))
			{
				pActor->RemAffect("sneak");
				pActor->Send("Whilst wearing %s you cannot move silently.\n\r", (*it)->m_sName.c_str());
			}

			return true;
		}
	}

	pActor->Send("&RYou don't seem to be carrying that item.\n\r");
	pActor->Send("&rType inventory to view what you are carrying currently.\n\r");
	pActor->Send("&rSyntax: wear <item name>\n\r");
	return true;
}

bool Shinobi::CmdRemove(Actor * pActor, string sArguments)
{
	// Check they entered an argument
	if (sArguments.empty())
	{
		pActor->Send("&RWhat do you wish to remove?\n\r");
		pActor->Send("&rType equipment to view what you are wearing currently.\n\r");
		pActor->Send("&rSyntax: remove <item name>\n\r");
		return true;
	}

	for (map<string, Item*>::iterator it = pActor->m_Equipment.begin(); it != pActor->m_Equipment.end(); it++)
	{
		if (StrContains((*it).second->m_sName, sArguments))
		{
			if (pActor->IsAffected("hide"))
			{
				pActor->RemAffect("hide");
				pActor->m_Variables->RemVar("hide_value");
				pActor->Send("Your action causes you to step from the shadows!\n\r");
			}

			if (pActor->IsAffected("meditation"))
				pActor->RemAffect("meditation");

			pActor->m_Inventory.push_back((*it).second);
			pActor->Send("You remove %s.\n\r", (*it).second->m_sName.c_str());
			pActor->m_pRoom->Send(pActor, "%s removes %s.\n\r", pActor->m_sName.c_str(), (*it).second->m_sName.c_str());
			pActor->m_Equipment.erase(it);

			// Check if item is disguise, if so remove affect and remove one point of quality
			if ((*it).second->m_Variables->Var("type") == "disguise")
			{
				pActor->DamageItem("worn", 1);
				pActor->RemAffect("disguise");
			}
			return true;
		}
	}

	pActor->Send("&RYou don't seem to be wearing that item.\n\r");
	pActor->Send("&rType equipment to view what you are wearing currently.\n\r");
	pActor->Send("&rSyntax: remove <item name>\n\r");
	return true;
}

bool Shinobi::CmdEquipment(Actor * pActor, string sArguments)
{
	if (pActor->m_Equipment.size() <= 0)
	{
		pActor->Send("&RYou are not wearing anything!\n\r");
		return true;
	}

	pActor->Send("Equipped:\n\r");

	for (map<string, Item*>::iterator it = pActor->m_Equipment.begin(); it != pActor->m_Equipment.end(); it++)
	{
		string sWear = "<";
		sWear.append((*it).first);
		sWear.push_back('>');
		pActor->Send(" %-10s %s%s\n\r", sWear.c_str(), (*it).second->HasFlag("broken") ? "(Broken)" : "", (*it).second->m_sName.c_str());
	}

	return true;
}

bool Shinobi::CmdInventory(Actor * pActor, string sArguments)
{
	if (pActor->m_Inventory.size() <= 0)
	{
		pActor->Send("&RYou are not carrying anything!\n\r");
		return true;
	}

	pActor->Send("Inventory:\n\r");

	map<string, int> ItemMap;

	for (list<Item*>::iterator it = pActor->m_Inventory.begin(); it != pActor->m_Inventory.end(); it ++)
	{
		ItemMap[(*it)->m_sName] += 1;
	}

	for (map<string, int>::iterator it = ItemMap.begin(); it != ItemMap.end(); it ++)
	{
		if ( (*it).second > 1)
			pActor->Send("[%d] %s\n\r", (*it).second, (*it).first.c_str());
		else
			pActor->Send("%s\n\r", (*it).first.c_str());
	}

	return true;
}

bool Shinobi::CmdExamine(Actor * pActor, string sArguments)
{
	Item * pItem = NULL;

	// Check they entered an argument
	if (sArguments.empty())
	{
		pActor->Send("&RWhat do you wish to examine?\n\r");
		pActor->Send("&rType inventory to view what you are carrying currently.\n\r");
		pActor->Send("&rSyntax: examine <item name>\n\r");
		return true;
	}

	// Check if the item is in their inventory
	for (list<Item*>::iterator it = pActor->m_Inventory.begin(); it != pActor->m_Inventory.end(); it++)
		if (StrContains((*it)->m_sName, sArguments))
			pItem = (*it);

	// Check the room
	if (!pItem)
	{
		for (list<Item*>::iterator it = pActor->m_pRoom->m_Items.begin(); it != pActor->m_pRoom->m_Items.end(); it++)
			if (StrContains((*it)->m_sName, sArguments))
				pItem = (*it);
	}

	if (pItem)
	{
		pActor->Send("You examine %s:\n\r", pItem->m_sName.c_str());
		pActor->Send("%s\n\r", pItem->m_Variables->Var("desc").c_str());
		pActor->Send("The item type is: %s\n\r", pItem->m_Variables->Var("type").c_str());

		if (pItem->m_Flags.size() > 0)
		{
			pActor->Send("Item flags: ");
			for (map<string, int>::iterator jt = pItem->m_Flags.begin(); jt != pItem->m_Flags.end(); jt++)
				pActor->Send("%s ", ToCapital((*jt).first).c_str());
			pActor->Send("\n\r");
		}

		if (pActor->IsAffected("meditation"))
			pActor->RemAffect("meditation");

		if (pItem->m_Variables->Var("weapon_type").length() > 0)
			pActor->Send("The item is a %s\n\r", pItem->m_Variables->Var("weapon_type").c_str());

		if (pItem->m_Variables->Var("wearloc").length() > 0)
			pActor->Send("It can be worn on the %s\n\r", pItem->m_Variables->Var("wearloc").c_str());

		if (pItem->m_Variables->VarI("max_quality") > 0)
			pActor->Send("The item's current quality is %d\n\r", pItem->m_Variables->VarI("curr_quality"));

		if (pItem->m_Variables->HasVar("decay_timer"))
			pActor->Send("The item will decay%s.\n\r", pItem->m_Variables->VarI("decay_timer") <= 60 ? " imminently" : "");

		// Contents
		if (pItem->m_Contents.size() > 0)
		{
			pActor->Send("The item contains:\n\r");
			for (list<Item*>::iterator it = pItem->m_Contents.begin(); it != pItem->m_Contents.end(); it++)
				pActor->Send(" * %s\n\r", (*it)->m_sName.c_str());
		}

		pActor->m_pRoom->Send(pActor, "%s examines %s.\n\r", pActor->m_sName.c_str(), pItem->m_sName.c_str());
		return true;
	}

	// If not, is it the room?
	pActor->Send("&RYou don't seem to be able to find that item.\n\r");
	pActor->Send("&rType inventory to view what you are carrying currently.\n\r");
	pActor->Send("&rSyntax: examine <item name>\n\r");
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// SHINOBI COMMANDS
/////////////////////////////////////////////////////////////////////////////////////////////////////

// Command - Sneak
// Enable silent movement of the Shinobi
bool Shinobi::CmdSneak(Actor * pActor, string sArguments)
{
	if (pActor->IsAffected("sneak"))
	{
		pActor->Send("&RYou are already moving silently!\n\r");
		return true;
	}

	// Sneaking whilst wearing anything in the body location is a no no
	if (pActor->GetEquipped("body"))
	{
		pActor->Send("&RYou cannot move silently whilst wearing items on the body.\n\r");
		pActor->Send("&rHint: Remove any equipment worn on the body using the remove command.\n\r");
		return true;
	}

	if (pActor->IsAffected("meditation"))
		pActor->RemAffect("meditation");

	// Not sneaking - so sneak
	pActor->AddAffect("sneak");
	pActor->Send("&GYou begin to move silently.\n\r");
	return true;
}

bool Shinobi::CmdMeditate(Actor * pActor, string sArguments)
{
	if (pActor->IsAffected("meditation"))
	{
		pActor->Send("&RYou are already meditating, it doesn't get more zen than this!\n\r");
		return true;
	}

	if (pActor->IsFighting())
	{
		pActor->Send("&RFind your inner peace in the middle of battle? I think not!\n\r");
		return true;
	}

	if (pActor->IsAffected("hide"))
	{
		pActor->RemAffect("hide");
		pActor->m_Variables->RemVar("hide_value");
		pActor->Send("Your action causes you to step from the shadows!\n\r");
	}

	// Focus is lost when meditating
	if (pActor->IsAffected("focus"))
		pActor->RemAffect("focus");

	// Meditation will 'tick' every 6 seconds
	pActor->AddAffect("meditation", 0, 6);
	pActor->Send("&GYou settle down and begin to meditate.\n\r");
	pActor->m_pRoom->Send(pActor, "%s settles down and begins to meditate.\n\r", pActor->m_sName.c_str());
	return true;
}

bool Shinobi::CmdHide(Actor * pActor, string sArguments)
{
	if (pActor->m_Variables->VarI("curr_attr_5") < 1)
	{
		pActor->Send("&RYou lack the required spirit to perform that action, try meditating first.\n\r");
		return true;
	}

	if (pActor->IsAffected("hide"))
	{
		pActor->m_Variables->Set("hide_value", pActor->m_Variables->VarI("curr_attr_4") + Main::rand_range(1, 6));
		pActor->Send("You adjust your position within the shadows as silently as possible.\n\r");
		pActor->m_Variables->AddToVar("curr_attr_5", -1);
		return true;
	}

	if (pActor->IsAffected("meditation"))
		pActor->RemAffect("meditation");

	// Not hidden - so hide them
	pActor->AddAffect("hide");
	pActor->Send("&GYou slip silently into the shadows.\n\r");
	pActor->m_Variables->AddToVar("curr_attr_5", -1);
	pActor->m_Variables->Set("hide_value", pActor->m_Variables->VarI("curr_attr_4") + Main::rand_range(1, 6));
	return true;
}

// Command - Visible
// Remove any sneak or hide affects on the character
bool Shinobi::CmdVisible(Actor * pActor, string sArguments)
{
	if (pActor->IsAffected("sneak"))
	{
		pActor->RemAffect("sneak");
		pActor->Send("You stop moving silently.\n\r");
	}

	if (pActor->IsAffected("hide"))
	{
		pActor->RemAffect("hide");
		pActor->m_Variables->RemVar("hide_value");
		pActor->Send("You step from the shadows, revealing yourself.\n\r");
		pActor->m_pRoom->Send(pActor, "%s steps from the shadows, revealing themself.\n\r", pActor->m_sName.c_str());
	}

	return true;
}

// Command - Focus
// Temporarily increase a Shinobi's Wisdom at the expense of spirit
bool Shinobi::CmdFocus(Actor * pActor, string sArguments)
{
	if (pActor->m_Variables->VarI("curr_attr_5") < 1)
	{
		pActor->Send("&RYou lack the required spirit to perform that action, try meditating first.\n\r");
		return true;
	}

	// If already focused - increase the focus modifier
	if (pActor->IsAffected("focus"))
	{
		if (pActor->Attribute("attr_3") >= 15)
		{
			pActor->Send("&RYou cannot improve your focus any further.\n\r");
			return true;
		}

		if (pActor->Attribute("attr_3") >= 13)
		{
			int nInc = 15 - pActor->Attribute("attr_3");
			pActor->IncreaseAffect("focus", nInc);			// Increase the modifier
			pActor->IncreaseTime("focus", 0, nInc * 10);	// Gain an additional 10 seconds
		}
		else
		{
			pActor->IncreaseAffect("focus", 3);
			pActor->IncreaseTime("focus", 0, 30);
		}

		if (pActor->IsAffected("meditation"))
			pActor->RemAffect("meditation");
		if (pActor->IsAffected("hide"))
			pActor->RemAffect("hide");

		pActor->m_Variables->AddToVar("curr_attr_5", -1);	// Use some spirit
		pActor->Send("&GYou expend your spirit to further improve your focus.\n\r");
		return true;
	}

	if (pActor->IsAffected("meditation"))
		pActor->RemAffect("meditation");
	if (pActor->IsAffected("hide"))
		pActor->RemAffect("hide");

	// Focus
	pActor->AddAffect("focus", AFF_ATTR_3, 3, 0, 30);
	pActor->Send("&GYour vision becomes suddenly clearer as you expend your spirit.\n\r");
	pActor->m_Variables->AddToVar("curr_attr_5", -1);	// Use some spirit
	return true;

	return true;
}

// Command - Suicide
// Delete a character
bool Shinobi::CmdSuicide(Actor * pActor, string sArguments)
{
	if (sArguments.empty())
	{
		pActor->Send("&RSuicide is an ignoble death, are you sure you wish to do this?\n\r");
		pActor->Send("&RTo continue, type: suicide <password>\n\r");
		pActor->m_pRoom->Send(pActor, "%s is contemplating suicide!\n\r", pActor->m_sName.c_str());
		pActor->m_Variables->Set("suicide", 1);
	}
	else
	{
		// Check
		if (pActor->m_Variables->VarI("suicide") == 1)
		{
			Player * pPlayer = (Player*) pActor;
			// Already entered the command with no arguments, check the password
			string sPass = SqliteDbase::Get()->GetString("accountdata", "password", "name", pPlayer->m_pAccount->m_sName);
			string sResult(crypt(sArguments.c_str(), pPlayer->m_pAccount->m_sName.c_str()));

			if (sResult == sPass)
			{
				// Correct password
				pActor->m_Variables->RemVar("suicide");
				pActor->Send("&RYou have commited suicide!\n\r");
				pActor->CreateCorpse();
				DeletePlayer((Player*)pActor);
				pActor->m_pRoom->Send(pActor, "%s has commited suicide!\n\r", pActor->m_sName.c_str());
				delete pActor;
				return true;
			}
			else
			{
				pActor->m_Variables->RemVar("suicide");
				pActor->Send("&RIncorrect password!\n\r");
				pActor->m_pWorld->Console("%s entered incorrect password for suicide attempt.", pActor->m_sName.c_str());
				return true;
			}
		}
		else
		{
			pActor->Send("&RIf you want to commit suicide you must enter the command with no arguments.\n\r");
			return true;
		}
	}
	return true;
}

bool Shinobi::CmdRetire(Actor * pActor, string sArguments)
{
	return true;
}

bool Shinobi::CmdPlaydead(Actor * pActor, string sArguments)
{
	return true;
}

// Command - Throw
// Throw a shuriken at a target
bool Shinobi::CmdThrow(Actor * pActor, string sArguments)
{
	// Must have a target first
	if (!pActor->m_pTarget)
	{
		pActor->Send("&RTo throw anything you need to first select a target!\n\r");
		pActor->Send("&rSyntax: look <name>\n\r");
		return true;
	}

	// Is the target immortal TODO
	if (pActor->m_pTarget->HasFlag("immortal"))
	{
		pActor->Send("&REven the Emperor himself couldn't kill them!\n\r");
		return true;
	}

	// Check not affected by thrown_shuriken (2 secs per throw!) this is set by the quality
	// of the thrown item
	if (pActor->IsAffected("thrown"))
	{
		pActor->Send("&RYou've only just thrown the last one!\n\r");
		return true;
	}

	if (sArguments.empty())
		sArguments = "shuriken";

	Item * pShuriken = FindItem(sArguments, pActor);

	// Check has Shuriken in inventory
	if (pShuriken)
	{
		if (pActor->m_pTarget->m_nType == _NPC)
		{
			Item * pWield = pActor->GetEquipped("hands");

			if (pWield)
				pActor->m_Equipment["hands"] = pShuriken;

			CmdKill(pActor, "");

			if (pWield)
				pActor->m_Equipment["hands"] = pWield;
		}
		else
		{
			pActor->Act("", "", pActor->m_pTarget, pShuriken, NULL, "$n fling$p a shuriken, hitting $N!");
			// Deal 5 * Item Quality + Agility damage to target
			int nDam = pActor->Attribute("attr_2") + (5 * pShuriken->m_Variables->VarI("curr_quality"));

			if (m_Variables->VarI("combat_debug") == 1)
				pActor->Send("Combat: Throw Damage == %d\n\r", nDam);

			if (pActor->m_pTarget->Damage(nDam))
			{
				pActor->m_pTarget->Die(pActor);
				pActor->m_pTarget = NULL;
//				pActor->m_pTarget->Send("&RYou are DEAD!\n\r");
//				pActor->m_pRoom->Send(pActor->m_pTarget, "&R%s is DEAD!\n\r", pActor->m_pTarget->m_sName.c_str());
				return true;
			}

		}

		// Add Affect and remove the item
		pActor->AddAffect("thrown", 0, pShuriken->m_Variables->VarI("curr_quality"));
		pActor->m_Inventory.remove(pShuriken);
		delete pShuriken;
	}
	else
	{
		pActor->Send("&RYou don't seem to be carrying that item.\n\r");
		pActor->Send("&rSyntax: thrown <item>\n\r");
		pActor->Send("&r        With no arguments will default to Shuriken\n\r");
		return true;
	}


	return true;

}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// COMBAT COMMANDS
/////////////////////////////////////////////////////////////////////////////////////////////////////

// Command - Kill
// Initiate combat with an NPC you have examined
bool Shinobi::CmdKill(Actor * pActor, string sArguments)
{
	if (!pActor->m_pTarget)
	{
		pActor->Send("&RTo kill a target you must first examine them!\n\r");
		pActor->Send("&rSyntax: look <name>\n\r");
		return true;
	}

	// Has a valid target, is it a NPC?
	if (pActor->m_pTarget->m_nType != _NPC)
	{
		pActor->Send("&RKill them? Why not trying fighting them instead!\n\r");
		pActor->Send("&rSyntax: fight <player>\n\r");
		return true;
	}

	// Is the target immortal TODO
	if (pActor->m_pTarget->HasFlag("immortal"))
	{
		pActor->Send("&REven the Emperor himself couldn't kill them!\n\r");
		return true;
	}

	// Is this a Swish THUD moment?
	if (pActor->m_pTarget->m_Variables->VarI("warrior_rating") > 0)
	{
		// NPC Combat - Play has attacked a Warrior type player
		// Base Hit chance = 70%
		//	-- Add Strength attribute
		//	-- Add Agility attribute
		//	-- Add Weapon Quality
		// 	-- Subtract Warrior Rating x 10
		// Roll LESS than a D100

		int nBase = 70;
		nBase += pActor->Attribute("attr_1");
		nBase += pActor->Attribute("attr_2");
		nBase += pActor->Weapon("curr_quality");
		nBase -= (pActor->m_pTarget->m_Variables->VarI("warrior_rating") * 10);

		int nD100 = Main::rand_range(1, 100);

		// Min chance of 5, max of 95
		if (nBase > 95)
			nBase = 95;

		if (nBase < 5)
			nBase = 5;

		if (m_Variables->VarI("combat_debug") == 1)
			pActor->Send("Combat: Base chance to hit == %d D100(%d)\n\r", nBase, nD100);


		if (nBase > nD100)
		{
			// MISS!
			int nDam = pActor->m_pTarget->m_Variables->VarI("warrior_rating") * 10;

			if (m_Variables->VarI("combat_debug") == 1)
				pActor->Send("Combat: Damage taken == %d\n\r", nDam);

			if (pActor->Armour("curr_quality") > 0)
				nDam = float(nDam) * (float(pActor->Armour("curr_quality")) / 100.0);

			if (m_Variables->VarI("combat_debug") == 1)
				pActor->Send("Combat: After Armour == %d\n\r", nDam);

			if (pActor->Damage(nDam))
			{
				pActor->Die(pActor->m_pTarget);
				return true;
			}
		}

	}

	// Give a combat message
	pActor->CombatMessage(pActor->m_pTarget, true);
	// Give a death cry for the NPC
	pActor->m_pTarget->DeathCry(pActor);
	// Generate any loot
	pActor->m_pTarget->DetermineLoot();
	// Create the corpse and kill the Actor
	pActor->m_pTarget->Die(pActor, true);
	// No more calling m_pTarget is the memory has been deleted within the Die function
	// Reset their target
	pActor->m_pTarget = NULL;
	// Increase the players kills
	pActor->m_Variables->AddToVar("npc_kills", 1);

	return true;

}


// Command - Fight
// Initiates a PvP Engagement
bool Shinobi::CmdFight(Actor * pActor, string sArguments)
{
	// If they do not have a target prompt them
	if (!pActor->m_pTarget)
	{
		pActor->Send("&RYou must first target or look at a player to initiate a fight.\n\r");
		pActor->Send("&rSyntax: target <player>\n\r");
		pActor->Send("&r        look <player>\n\r");
		return true;
	}

	// If the target is an NPC use the KILL command to handle the combat
	if (pActor->m_pTarget->m_nType == _NPC)
	{
		// Use the kill command instead
		return CmdKill(pActor, sArguments);
	}

	// Has a valid PC target
	// Check they are not already in combat
	if (pActor->m_pTarget->IsFighting())
	{
		pActor->Send("&RThey are already in the middle of a fight!\n\r");
		return true;
	}

	// Not fighting - Start a royal rumble!
	ShinobiCombat * pCombat = new ShinobiCombat();
	pCombat->m_pAttacker = pActor;
	pCombat->m_pDefender = pActor->m_pTarget;

	// Link the actors to this object
	pActor->m_pCombat = pCombat;
	pActor->m_pTarget->m_pCombat = pCombat;

	// Remove any affects that combat nullifies
	if (pActor->IsAffected("meditation"))
		pActor->RemAffect("meditation");
	if (pActor->IsAffected("hide"))
		pActor->RemAffect("hide");
	if (pActor->IsAffected("focus"))
		pActor->RemAffect("focus");
	if (pActor->m_pTarget->IsAffected("meditation"))
		pActor->m_pTarget->RemAffect("meditation");
	if (pActor->m_pTarget->IsAffected("hide"))
		pActor->m_pTarget->RemAffect("hide");
	if (pActor->m_pTarget->IsAffected("focus"))
		pActor->m_pTarget->RemAffect("focus");



	// Add the combat to the Combat Manager
	m_Combat.push_back(pCombat);

	if (pActor->m_Variables->HasVar("combat_ambush"))
		pActor->Send("You prepare to ambush your target\n\r");
	else
		pActor->Act(pActor->m_pTarget, "$n attack$p $N!");
	return true;
}

// Command - Stance
// Toggles between a defensive and offensive stance
bool Shinobi::CmdStance (Actor * pActor, string sArguments)
{
	if (pActor->m_Variables->HasVar("combat_stance"))
	{
		pActor->Send("&rYou can only change your stance once per round of combat!\n\r");
		return true;
	}

	if (pActor->m_Variables->VarI("stance") == COMBAT_OFFENSIVE)
		pActor->m_Variables->Set("stance", COMBAT_DEFENSIVE);
	else
		pActor->m_Variables->Set("stance", COMBAT_OFFENSIVE);

	if (pActor->IsFighting())
		pActor->m_Variables->Set("combat_stance", 1);

	pActor->Act("$n adopt$p a %s stance.", pActor->m_Variables->VarI("stance") == COMBAT_OFFENSIVE ? "offensive" : "defensive");
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// PLAYER COMBAT ACTIONS
/////////////////////////////////////////////////////////////////////////////////////////////////////

// Anchillary Function to check the numerous requirements for using a combat move within combat
bool Shinobi::CmdCombatMove(Actor * pActor, string sArguments)
{
	// Check is in combat
	if (!pActor->m_pCombat)
	{
		pActor->Send("&RBut, you aren't fighting anyone!\n\r");
		pActor->Send("&rTo start a fight you must first target an opponent then fight them.\n\r");
		pActor->Send("&rSyntax: look/target <player>\n\r");
		pActor->Send("&r        fight\n\r");
		return false;
	}

	// Check hasn't already input a move
	if (pActor->m_Variables->HasVar("combat_move"))
	{
		pActor->Send("&RYou have already selected your move this round.\n\r");
		return false;
	}

	// Strip out the first word
	string sMove = "";

	for (size_t i = 0; i < sArguments.length(); i++)
	{
		if (sArguments.at(i) == ' ')
		{
			sMove = sArguments.substr(0, i);
			break;
		}
	}

	if (sMove.empty())
		sMove = sArguments;

	bool bFound = false;
	// Check move is a valid combat move
	for (vector<string>::iterator it = pActor->m_pCombat->m_Moves.begin(); it != pActor->m_pCombat->m_Moves.end(); it++)
	{
		if (StrEquals((*it), sMove))
			bFound = true;
	}

	if (!bFound)
	{
		pActor->Send("&R%s is not valid combat move.\n\r", sMove.c_str());
		pActor->Send("&rValid moves:\n\r");
		int nCount = 0;
		for (vector<string>::iterator it = pActor->m_pCombat->m_Moves.begin(); it != pActor->m_pCombat->m_Moves.end(); it++)
		{
			if (nCount > 80)
			{
				nCount = 0;
				pActor->Send("\n\r");
			}

			pActor->Send("%s ", (*it).c_str());
			nCount += (1 + (*it).length());
		}
		return true;
	}

	if (StrEquals(sMove, "dodge") && (pActor->IsAffected("cornered")))
	{
		pActor->Send("&RYou are cornered, theres nowhere for you to dodge!\n\r");
		return true;
	}

	if (StrEquals(sMove, "strike") && (pActor->IsAffected("overextended")))
	{
		pActor->Send("&RYou are overextended, you can't strike!\n\r");
		return true;
	}

	pActor->m_Variables->Set("combat_move", sMove);
	pActor->m_Variables->Set("combat_move_time", GameServer::Time());

	pActor->Send("&G%s selected.\n\r", ToCapital(sMove).c_str());
	return true;
}

bool Shinobi::CmdSmokebomb(Actor * pActor, string sArguments)
{

	return true;
}

