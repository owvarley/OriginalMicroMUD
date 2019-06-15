/*
 * GameWorld.cpp
 *
 *  Created on: Mar 15, 2012
 *      Author: Gnarls
 */

#include "GameWorld.h"
#include "SqliteDbase.h"
#include "Player.h"
#include "Combat.h"
#include "Areas/Item.h"
#include "Variable.h"
#include "Libraries/tinyxml2.h"
#include <iostream>

using namespace tinyxml2;

GameWorld::GameWorld()
{
	// TODO Auto-generated constructor stub
	m_Variables = new VariableManager();
}

GameWorld::~GameWorld()
{
	// TODO Auto-generated destructor stub
}

void GameWorld::Update()
{

}

void GameWorld::Interpret(GameSocket * pSocket, std::string sInput)
{
	return;
}

void GameWorld::PlayMenu(GameSocket * pSocket)
{
	return;
}

void GameWorld::Splash(GameSocket * pSocket)
{
	return;
}

void GameWorld::Nanny(GameSocket * pSocket, std::string sInput)
{

}

bool GameWorld::Move(GameSocket * pSocket, std::string sDirection)
{
	return false;
}

void GameWorld::Send(const std::string sMsg, ...)
{
    char buf[MAX_STRING_LENGTH*2];
    va_list args;

    va_start(args, sMsg);
    vsprintf(buf, sMsg.c_str(), args);
    va_end(args);

    string sEdit(buf);

	for (map<int, Player*>::iterator it = m_Players.begin(); it != m_Players.end(); it ++)
	{
		(*it).second->Send("[%s] %s\n\r", m_sName.c_str(), sEdit.c_str());
	}
}

void GameWorld::Send(Actor * pActor, const std::string sMsg, ...)
{
    char buf[MAX_STRING_LENGTH*2];
    va_list args;

    va_start(args, sMsg);
    vsprintf(buf, sMsg.c_str(), args);
    va_end(args);

    string sEdit(buf);

	for (map<int, Player*>::iterator it = m_Players.begin(); it != m_Players.end(); it ++)
	{
		if ((*it).second == pActor)
			continue;

		(*it).second->Send("[%s] %s\n\r", m_sName.c_str(), sEdit.c_str());
	}
}

void GameWorld::Console(const std::string sMsg, ...)
{

}

bool GameWorld::Command(Actor * pActor, string sCommand, string sArguments)
{
	return false;
}



// Load the Combat Messages for the GameWorld from XML
bool GameWorld::LoadCombatMessages()
{
	XMLDocument doc;
	doc.LoadFile( "data/combatmessages.xml" );

	if (doc.Error())
	{
		doc.PrintError();
		return false;
	}

	XMLElement * pText = doc.FirstChildElement( "GameWorld" );

	// Parse the XML file for data
	while ( pText )
	{
		string sName = pText->Attribute("name");

		// Find a <GameWorld> tag that has our name as an attribute
		if (!sName.empty() && StrEquals(sName, m_sName))
		{

			for (XMLElement * pElement = pText->FirstChildElement(); pElement; pElement = pElement->NextSiblingElement())
			{
				// Get the name of the Element
				string sMove = pElement->Name();
				string sWeapon = "";
				if (pElement->Attribute("weapon") != 0)
					sWeapon = pElement->Attribute("weapon");
				string sState = "";
				if (pElement->Attribute("state") != 0)
					sState = pElement->Attribute("state");
				string sMessage = pElement->GetText();

				CombatMessage * pMsg = new CombatMessage(sMove, sWeapon, sState, sMessage);
				m_CombatMsgs.push_back(pMsg);
			}
		}


		pText = pText->NextSiblingElement( "GameWorld" );
	}


	return true;
}

// Load all areas, items and NPCs for this GameWorld from our database
void GameWorld::LoadAreas()
{

}

void GameWorld::SendMSSP(GameSocket * pSocket)
{

}

Room * GameWorld::StartingRoom()
{
	return NULL;
}

void GameWorld::CreatePlayer(Player * pActor)
{

}

Item * GameWorld::FindItem(string sName, Item * pItem)
{
	for (list<Item*>::iterator jt = pItem->m_Contents.begin(); jt != pItem->m_Contents.end(); jt++)
		if (StrContains((*jt)->m_sName, sName))
			return (*jt);

	return NULL;
}

Item * GameWorld::FindItem(string sName, Actor * pActor)
{
	for (list<Item*>::iterator it = pActor->m_Inventory.begin(); it != pActor->m_Inventory.end(); it++)
	{
		// Did we find a match?
		if (StrContains((*it)->m_sName, sName))
			return (*it);
	}

	return NULL;
}

Item * GameWorld::FindItem(string sName, Room * pRoom)
{
	for (list<Item*>::iterator it = pRoom->m_Items.begin(); it != pRoom->m_Items.end(); it++)
	{
		// Did we find a match?
		if (StrContains((*it)->m_sName, sName))
			return (*it);
	}

	return NULL;
}

void GameWorld::DeletePlayer (Player * pActor)
{
	// Delete them from the Array but also delete their details
	RemPlayer(pActor);
}

void GameWorld::AddPlayer (Player * pActor)
{
	m_Players[pActor->m_nId] = pActor;
}

void GameWorld::RemPlayer (Player * pActor)
{
	m_Players.erase(pActor->m_nId);
}


string GameWorld::AttrName(int nId)
{
	return "";
}

string GameWorld::AffectName(int nAffect)
{
	return "";
}

void GameWorld::CombatManager()
{
	for (vector<Combat*>::iterator it = m_Combat.begin(); it != m_Combat.end(); it++)
		(*it)->Update();
}

// Generate a Combat message for an Actor
void GameWorld::SendCombatMsg (Actor * pActor, Actor * pVictim, string sMove, string sState)
{
	string sWeapon = "fist";
	Item * pActWield = NULL;
	Item * pVicWield = NULL;

	if (pActor->GetWeapon())
	{
		pActWield = pActor->GetWeapon();
		sWeapon = pActWield->m_Variables->Var("weapon_type");
	}

	if (pVictim && pVictim->GetWeapon())
		pVicWield = pVictim->GetWeapon();

	pActor->Act("", "", pVictim, pActWield, pVicWield, CombatMsg(sMove, sWeapon, sState));
	return;
}

// Combat Messages are defined in the XML file combatmessages.xml the format they are saved in
// is detailed within the XML but roughly follows that the name of the move is the Element name,
// if the message is specific to a state (i.e. Hit or Miss) then this is appended with a state attribute
// Along the same lines if the message is for a specific weapon type this is appended using the Weapon
// attribute tag. Should no message be available for the given weapon, a generic message for that type
// will be used
//
// The CombatMsg function will return a valid Combat Message using the combination of inputs it receives
// this is called by SendCombatMsg to compile a message for an Actor
string GameWorld::CombatMsg(string sMove, string sWeapon, string sState)
{
	// Our first list will be all the strings that match this move, regardless of weapon or state
	vector<string> MatchedStrings;
	// This list will contain all the strings that match our move and the weapon used
	vector<string> MatchedWeaponStrings;

	// Compile lists of valid strings
	for (vector<CombatMessage*>::iterator it = m_CombatMsgs.begin(); it != m_CombatMsgs.end(); it++)
	{
		if (sState.empty())
		{
			// Find a matching move for when we have not supplied a state
			if (StrEquals((*it)->m_sMove, sMove) && (*it)->m_sState.empty())
			{
				if ((*it)->m_sWeapon.empty())
					MatchedStrings.push_back((*it)->m_sMessage);

				// Add the string if it is specific to our weapon type, or it is valid for all types
				// the only exception is for fist attacks, the messages flagged all are not suitable for
				// this as they use $o which will cause erroneous messages
				if (StrEquals((*it)->m_sWeapon, sWeapon) || (StrEquals((*it)->m_sWeapon, "all") && sWeapon != "fist"))
					MatchedWeaponStrings.push_back((*it)->m_sMessage);
			}
		}
		else
		{
			// Find a matching move for when we have not supplied a state
			if (StrEquals((*it)->m_sMove, sMove) && StrEquals((*it)->m_sState, sState))
			{
				if ((*it)->m_sWeapon.empty())
					MatchedStrings.push_back((*it)->m_sMessage);

				if (StrEquals((*it)->m_sWeapon, sWeapon) || (StrEquals((*it)->m_sWeapon, "all") && sWeapon != "fist"))
					MatchedWeaponStrings.push_back((*it)->m_sMessage);
			}
		}
	}

	// If we asked for a specific message for a weapon, check that we have one and return it
	if (!sWeapon.empty())
	{
		if (MatchedWeaponStrings.size() > 0)
			return MatchedWeaponStrings.at(Main::rand_range(0, MatchedWeaponStrings.size()));
	}

	if (MatchedStrings.size() > 0)
		return MatchedStrings.at(Main::rand_range(0, MatchedStrings.size()));

	// Reaching this area of the function means that the requested string does not exist
	// within the database and should be added

	this->Console("No message for Move: %s, Weapon: %s, State: %s\n\r", sMove.c_str(), sWeapon.c_str(), sState.c_str());
	return "No message available";
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
// GENERIC COMMANDS
/////////////////////////////////////////////////////////////////////////////////////////////////////

// Command - Roll
// Roll a dice and show the room the result
bool GameWorld::CmdRoll(Actor * pActor, string sArguments)
{
	if (sArguments.empty())
	{
		pActor->Send("&RYou must input a size of dice to roll\n\r");
		pActor->Send("&rSyntax: roll <6/8/10/100>\n\r");
		pActor->Send("&rFor example, to roll a D100, type roll 100\n\r");
		return true;
	}

	int nRoll = atoi(sArguments.c_str());

	if (nRoll < 0)
		nRoll = 1;

	pActor->Act("$n rolled a D%d, the result was %d.\n\r", nRoll, Main::rand_range(1, nRoll));
	return true;
}
