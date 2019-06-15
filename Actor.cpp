/*
 * Actor.cpp
 *
 *  Created on: Mar 25, 2012
 *      Author: Gnarls
 */

#include "Actor.h"
#include "Areas/Exit.h"
#include "Areas/Area.h"
#include "Main.h"
#include "SqliteDbase.h"
#include "GameSocket.h"
#include "Areas/Item.h"
#include "Affect.h"
#include "Player.h"
#include "Variable.h"
#include <string>

using namespace std;

Actor::Actor()
{
	m_pTarget = NULL;
	m_pFighting = NULL;
	m_pRoom = NULL;
	m_pWorld = NULL;
	m_pCombat = NULL;
	m_Variables = new VariableManager();
}

Actor::~Actor()
{
	delete m_Variables;
}

// Flag handling

bool Actor::HasFlag(string sFlag)
{
	if (m_Flags.find(sFlag) != m_Flags.end())
		return true;

	return false;
}

void Actor::AddFlag(string sFlag)
{
	if (HasFlag(sFlag))
		return;

	m_Flags[sFlag] = 1;
	return;
}

void Actor::RemFlag(string sFlag)
{
	if (HasFlag(sFlag))
		m_Flags.erase(sFlag);

	return;
}

// Determine the correct description that should be shown to all players standing in the
// same room as this actor
string Actor::RoomDesc(Actor * pLooker, bool bHidden)
{
	stringstream sDesc;

	// Check if this player can see through a player's disguise
	if (IsAffected("disguise"))
	{
		// To break disguise the looking player's Wisdom must exceed the disguised player's cunning
		if (Attribute("attr_4") > pLooker->Attribute("attr_3"))
		{
			// The disguise occupies the 'worn' slot of the player
			Item * pDisguise = GetEquipped("worn");

			if (pDisguise)
			{
				string sDisguiseName = pDisguise->m_sName;
				size_t found;
				found = sDisguiseName.find(" disguise");

				// Strip the extra space and the word disguise out to give us the string
				// that we will be disguising as
				sDisguiseName.replace(found, 9, "");

				if (!sDisguiseName.empty())
					sDesc << sDisguiseName << " is standing here.";
				else
					sDesc << "Peasant farmer is standing here,";

				return sDesc.str();
			}
		}
		else
			sDesc << "(Disguised) ";
	}

	if (!IsAffected("disguise") && m_nType == _PLAYER && ((Player*)this)->m_pSocket->m_Variables["afk"] == "yes")
		sDesc << "&R[AFK]&w ";

	if (bHidden)
		sDesc << "(Hidden) ";

	if (HasFlag("power"))
		sDesc << "&p(Power)&w";
	else if (HasFlag("super"))
		sDesc << "&P(Super)&w";

	// Add the perceived name of the player first
	sDesc << pLooker->Perception(this) << " ";

	// Check for bleeding, hiding, etc
	if (IsAffected("bleeding"))
		sDesc << "is bleeding freely";
	else if (IsAffected("bleeding heavily"))
		sDesc << "is bleeding from many severe wounds";
	else if (bHidden)
		sDesc << "is hiding here";
	else
		sDesc << "is standing here";

	// Check for a weapon
	if (GetWeapon())
		sDesc << " wielding " << GetWeapon()->m_sName;

	sDesc << ".";

	return sDesc.str();
}

// Perception returns the name of the Victim as viewed by the Actor, this caters for
// disguises, hidden characters, play dead, etc
string Actor::Perception(Actor * pVictim)
{
	if (!this)
		return pVictim->m_sName;

	// Disguises
	if (pVictim->IsAffected("disguise"))
	{
		// To break disguise the looking player's Wisdom must exceed the disguised player's cunning
		if (pVictim->Attribute("attr_4") > Attribute("attr_3"))
		{
			// The disguise occupies the 'worn' slot of the player
			Item * pDisguise = pVictim->GetEquipped("worn");

			if (pDisguise)
			{
				string sDisguiseName = pDisguise->m_sName;
				size_t found;
				found = sDisguiseName.find(" disguise");

				// Strip the extra space and the word disguise out to give us the string
				// that we will be disguising as
				sDisguiseName.replace(found, 9, "");

				if (!sDisguiseName.empty())
					return sDisguiseName;
				else
					return "Peasant farmer";
			}
		}
	}

	// Hidden players
	if (pVictim->IsAffected("hide") && (Attribute("attr_3") > pVictim->m_Variables->VarI("hide_value")))
		return pVictim->m_sName;

	// Play dead TODO

	return pVictim->m_sName;
}


void Actor::Send(const string &s, ...)
{
	return;
}

// ACT
// Compile a message and send it to two actors and the room they are in, using dollar sign designators
// the message is adapted to the viewpoint of either the Actor, the Victim or the Room.
// Capital dollar sign designators relate to the Victim, lowercase to the Actor
// e.g: $n == Actor's name, $N == Victim's name
// Valid dollar sign designators:
// $n -- Name
// $m -- him/her
// $e -- her/his
// $s -- his/her
// $z -- es
// $p -- s    For example: swing$p    Would show swing to the Actor (You swing) and
// $g -- ing  						  swings to both the Victim and Room (Owen swings)
// Full example: $n jump$p at $N, $E crouch$Z avoiding the blow!
// Actor:  You jump at Ken, he crouches avoiding the blow!
// Victim: Owen jumps at you, you crouch avoiding the blow!
// Room:   Owen jumps at Ken, he crouches avoiding the blow!
// Simples!
// Originally written for DNA but converted to work here.
void Actor::Act(string output, string output2, Actor * pVictim, Item * pItem, Item * pItem2, string sMessage, ...)
{
	char buf[MAX_STRING_LENGTH*2];
	va_list args;

	// Pull out any arguments from the string
	va_start(args, sMessage);
	vsprintf(buf, sMessage.c_str(), args);
	va_end(args);

	// Start with the full string for the room message
	stringstream sRoom;
	sRoom << buf << "\n\r";

	stringstream sCh;
	stringstream sVictim;

	sCh << buf;
	sVictim << buf;

	// Add the carriage return
	sCh << "\n\r";
	sVictim << "\n\r";

	string toCh = ActReplace(ACT_CH, this, pVictim, pItem, pItem2, sCh.str(), this);
	string toVi = ActReplace(ACT_VICTIM, this, pVictim, pItem, pItem2, sVictim.str(), pVictim);


	sCh.str("");
	sVictim.str("");

	if (output != "")
		sCh << output << " " << ToCapital(toCh);
	else
		sCh << ToCapital(toCh);

	if (output2 != "")
		sVictim << output2 << " " << ToCapital(toVi);
	else
		sVictim << ToCapital(toVi);

	this->Send(sCh.str());

	if (pVictim)
		pVictim->Send(sVictim.str());

	// Room message, replace ch for ch's name and victim for victim's name
	for (list<Actor*>::iterator it = m_pRoom->m_Actors.begin(); it != m_pRoom->m_Actors.end(); it++)
	{
		if (this && (*it) == this)
			continue;

		if (pVictim && pVictim == (*it))
			continue;

		(*it)->Send(ActReplace(ACT_ROOM, this, pVictim, pItem, pItem2, sRoom.str(), (*it)));
	}

	return;

}

void Actor::Act(string sMessage, ...)
{
	char buf[MAX_STRING_LENGTH*2];
	va_list args;

	// Pull out any arguments from the string
	va_start(args, sMessage);
	vsprintf(buf, sMessage.c_str(), args);
	va_end(args);

	Act("", "", NULL, NULL, NULL, buf);
	return;
}

void Actor::Act(Actor * pVictim, string sMessage, ...)
{
	char buf[MAX_STRING_LENGTH*2];
	va_list args;

	// Pull out any arguments from the string
	va_start(args, sMessage);
	vsprintf(buf, sMessage.c_str(), args);
	va_end(args);

	Act("", "", pVictim, NULL, NULL, buf);
	return;
}

void Actor::Heal (int nAmount)
{
	m_Variables->AddToVar("curr_hp", nAmount);

	if (m_Variables->VarI("curr_hp") > m_Variables->VarI("max_hp"))
		m_Variables->Set("curr_hp", m_Variables->VarI("max_hp"));
}

bool Actor::Damage (int nAmount)
{
	m_Variables->AddToVar("curr_hp", -nAmount);

	if (m_Variables->VarI("curr_hp") <= 0)
	{
		m_Variables->Set("curr_hp", 0);
		return true;
	}

	float fHp = 100.0 * ( float(m_Variables->VarI("curr_hp")) / float(m_Variables->VarI("max_hp")) );

	// Check for bleeding
	if (!IsAffected("bleeding") && fHp <= 66.0)
	{
		Act("$n $a bleeding!");
		AddAffect("bleeding");
	}

	if (IsAffected("bleeding") && fHp <= 33.0 && !IsAffected("bleeding heavily"))
	{
		RemAffect("bleeding");
		Act("$n $a bleeding heavily!");
		AddAffect("bleeding heavily");
	}

	return false;
}

int Actor::Armour(string sVal)
{
	if (!GetEquipped("body"))
		return 0;

	Item * pArmour = GetEquipped("body");

	if (!pArmour)
		return 0;

	if (pArmour->m_Variables->Var("type") != "armour")
		return 0;

	return pArmour->m_Variables->VarI(sVal);

}

int Actor::Weapon(string sVal)
{
	Item * pWeapon = GetEquipped("hands");

	if (!pWeapon)
			return 0;

	if (pWeapon->m_Variables->Var("type") != "weapon")
		return 0;

	return pWeapon->m_Variables->VarI(sVal);

}

Item * Actor::GetEquipped(string sWearloc)
{
	map<string, Item*>::iterator it = m_Equipment.find(sWearloc);

	if (it != m_Equipment.end())
		return (*it).second;

	return NULL;
}

// Cause damage to an item, any damage is subtracted from the item's quality
// should the quality drop to zero the item is destroyed, it cannot be worn,
// any worn items will remain worn but will be flagged broken
void Actor::DamageItem(string sWearloc, int nAmount)
{
	if (sWearloc != "hands" && sWearloc != "body")
		return;

	Item * pItem = GetEquipped(sWearloc);

	if (!pItem)
		return;

	pItem->m_Variables->AddToVar("curr_quality", -nAmount);

	if (pItem->m_Variables->VarI("curr_quality") <= 0)
	{
		pItem->m_Variables->Set("curr_quality", 0);
		pItem->AddFlag("broken");
		Send("%s is now broken.\n\r", pItem->m_sName.c_str());
		// Remove from wearloc, add to inventory
		m_Equipment.erase(sWearloc);
		m_Inventory.push_back(pItem);
	}
	else
		Send("%s loses a point of quality.\n\r", pItem->m_sName.c_str());

	return;
}

// Will attempt to move an Actor from their current room in a given direction. If the direction provided
// is not a cardinal direction then we will return false. In ALL other cases we will return true. By returning
// false we are telling the function that called Actor::Move that the direction provided was not a cardinal
// direction and there was no exit of the given name from the current room.
bool Actor::Move(string sDir)
{
	Room * pRoom = m_pRoom;

	if (!pRoom)
	{
		m_pWorld->Console("[Move] %s does not have a valid room object.\n\r", m_sName.c_str());
		return true;
	}

	// We are checking now if the given sDir is a valid exit from this room, if it is we will move the player
	// in that direction. If not the function does nothing, message handling is carried out by the
	// interpreter for such cases

	Exit * pExit = NULL;

	for (map<string, Exit*>::iterator it = pRoom->m_Exits.begin(); it != pRoom->m_Exits.end(); it++)
	{
		string sADir = Main::ConvertCardinal(sDir);
		if (StrContains((*it).second->m_sDir, sADir))
			pExit = (*it).second;
	}

	if (pExit)
	{
		// Only accept a move from a player in combat who is fleeing
		if (IsFighting() && m_Variables->Var("combat_move") != "flee")
		{
			Send("&RMove in the middle of a fight? Try fleeing!\n\r");
			Send("&rSyntax: flee\n\r");
			return true;
		}
		// Valid exit - move them through it

		if (IsFighting())
			Send("You flee %s!\n\r", pExit->m_sDir.c_str());
		else
			Send("You move %s.\n\r", pExit->m_sDir.c_str());

		Room * pNewRoom = m_pWorld->m_Areas[pRoom->m_pArea->m_nId]->m_Rooms[pExit->m_nTo];

		if (!pNewRoom)
		{
			m_pWorld->Console("[Move] No valid room object for target room (%d).\n\r", pExit->m_nTo);
			return true;
		}

		// Player is now going to move - remove the hide affect
		if (IsAffected("hide"))
		{
			RemAffect("hide");
			m_Variables->RemVar("hide_value");
		}

		// Stop them meditating
		if (IsAffected("meditation"))
			RemAffect("meditation");

		// Create a blood trail
		if (IsAffected("bleeding") || IsAffected("bleeding heavily"))
		{
			Item * pItem = new Item;
			pItem->m_sName = "A bloodtrail leading ";
			pItem->m_sName.append(pExit->m_sDir);
			pItem->m_Variables->Set("type", "bloodtrail");
			pItem->AddFlag("No Get");
			pItem->m_Variables->Set("decay_timer", 120);	// Lasts 2 minutes
			pItem->m_Variables->Set("desc", "A large number of drops of blood can be seen here. They form a trail following their owner of into the distance.");
			pRoom->m_Items.push_back(pItem);
		}

		pRoom->RemActor(this);


		// Send a message to the room the player is leaving
		for (list<Actor*>::iterator it = pRoom->m_Actors.begin(); it != pRoom->m_Actors.end(); it++)
		{
			// Compare wisdom of viewer vs cunning of sneaker
			if (IsAffected("sneak") && (*it)->Attribute("attr_3") > Attribute("attr_4"))
				(*it)->Send("%s sneaks %s.\n\r", (*it)->Perception(this).c_str(), pExit->m_sDir.c_str());
			else if (IsFighting())
				(*it)->Send("%s flees %s.\n\r", (*it)->Perception(this).c_str(), pExit->m_sDir.c_str());
			else if (!IsAffected("sneak"))
				(*it)->Send("%s leaves %s.\n\r", (*it)->Perception(this).c_str(), pExit->m_sDir.c_str());


		}

		// Send one also to the room the player is entering
		for (list<Actor*>::iterator it = pNewRoom->m_Actors.begin(); it != pNewRoom->m_Actors.end(); it++)
		{
			// Compare wisdom vs cunning of sneaker
			if (IsAffected("sneak") && (*it)->Attribute("attr_3") > Attribute("attr_4"))
				(*it)->Send("%s sneaks in from the %s.\n\r", (*it)->Perception(this).c_str(), Main::ReverseDirection(pExit->m_sDir).c_str());
			else if (IsFighting())
				(*it)->Send("%s runs in from the %s.\n\r", (*it)->Perception(this).c_str(), Main::ReverseDirection(pExit->m_sDir).c_str());
			else if (!IsAffected("sneak"))
				(*it)->Send("%s arrives from the %s.\n\r", (*it)->Perception(this).c_str(), Main::ReverseDirection(pExit->m_sDir).c_str());
		}

		pNewRoom->AddActor(this);
		m_pRoom = pNewRoom;
		m_pWorld->Command(this, "look", "");
		return true;
	}

	// If we got here the given direction wasn't a valid exit

	if (Main::IsCardinal(sDir))
	{
		Send("&RThere is no exit in that direction, type look to see valid exits.\n\r");
		return true;	// True to indicate we have sent a message for this error already
	}

	return false;
}

void Actor::LoadNPC()
{
	// Don't try any load a non NPC
	if (m_nType != _NPC)
		return;

	// Don't load any NPC without an ID
	if (m_nId == 0)
		return;

	CppSQLite3Query query = SqliteDbase::Get()->GetQuery("select * from npcdata where npcid=%d", m_nId);

	// Set the NPCs basic values initially
	while (!query.eof())
	{
		m_sName = query.getStringField("name");
		query.nextRow();
	}

	// Add NPC variables in
	query = SqliteDbase::Get()->GetQuery("select variable, value, number from npcvariables where npcid=%d", m_nId);

	while (!query.eof())
	{
		// Column 0 == Variable
		// Column 1 == Value
		// Column 2 == Number (If == 1 then this is a number)

		if (atoi(query.fieldValue(2)) > 0)
			m_Variables->AddVar(query.fieldValue(0), atoi(query.fieldValue(1)));
		else
			m_Variables->AddVar(query.fieldValue(0), query.fieldValue(1));

		query.nextRow();
	}

	// Add any NPC flags in
	query = SqliteDbase::Get()->GetQuery("select flag from npcflags where npcid=%d", m_nId);

	while (!query.eof())
	{
		m_Flags[query.getStringField("flag")] = 1;	// 1 == NPC has flag
		query.nextRow();
	}

	// Finally we need to create the NPC loot table. The npcitems table contains all the possible
	// drops from this NPC along with the percentage chance of it dropping on death. To front load
	// this and prevent access to the database during gameplay we will store the details for each
	// item in the m_Loot structure of the Actor. Loot table is a map <int, int> the first int is
	// the itemid and the second int is the chance of it dropping.
	query = SqliteDbase::Get()->GetQuery("select * from npcitems where npcid=%d", m_nId);

	// npcitems - npcid, itemid, chance
	while (!query.eof())
	{
		m_Loot[query.getIntField("itemid")] = query.getIntField("chance");
		query.nextRow();
	}

}

// Combat Message - Give a message signaling an attack against another Actor
void Actor::CombatMessage(Actor * pTarget, bool bDeath)
{
	string sMessage;
	string sRoom;

	// Find out the weapon type
	Item * pWeapon = GetEquipped("hands");

	if (pWeapon)
	{
		if (pWeapon->m_Variables->Var("weapon_type") == "sword")
			Act("", "", pTarget, pWeapon, NULL, "$n slash$e out with $s sword, cutting it into $N's face%s\n\r", bDeath ? " killing them!" : ".");
		else if (pWeapon->HasFlag("Shuriken"))
			Act("", "", pTarget, pWeapon, NULL, "$n fling$p a shuriken into $N's face%s\n\r", bDeath ? " killing them!" : ".");
	}
	else
	{
		Act("", "", pTarget, pWeapon, NULL, "$n swing$p $s fist, slamming it into $N's face%s\n\r", bDeath ? " killing them!" : ".");
	}

	return;
}

bool Actor::IsFighting()
{
	if (m_pCombat)
		return true;

	return false;
}


// Move to a completely random location within the current area
void Actor::Respawn()
{
	int nOldRoom = m_pRoom->m_nId;
	int nArea = m_pRoom->m_pArea->m_nId;

	string sMsg = m_pWorld->m_Variables->Var("respawn_msg");
	string sChMsg = m_pWorld->m_Variables->Var("respawn_ch_msg");

	if (!sMsg.empty())
		sMsg = ActReplace(ACT_ROOM, this, NULL, NULL, NULL, sMsg, this);

	Send("\n\r%s\n\r", sChMsg.c_str());

	// Remove them from the current room
	m_pRoom->RemActor(this);
	m_pRoom = NULL;

	for (map<int, Area*>::iterator it = m_pWorld->m_Areas.begin(); it != m_pWorld->m_Areas.end(); it++)
	{
		if ((*it).second->m_nId == nArea)
		{
			int nRandRoom = Main::rand_range(0, (*it).second->m_Rooms.size());
			map<int, Room*>::iterator rt = (*it).second->m_Rooms.begin();
			advance (rt, nRandRoom);

			while ((*rt).second->m_nId == nOldRoom || (*rt).second->m_Exits.size() <= 0)
			{
				nRandRoom = Main::rand_range(0, (*it).second->m_Rooms.size());
				rt = (*it).second->m_Rooms.begin();
				advance (rt, nRandRoom);
			}

			if (!sMsg.empty())
				(*rt).second->Send("%s\n\r", sMsg.c_str());
			else
				(*rt).second->Send("%s appears in the room.\n\r", m_sName.c_str());

			m_pRoom = (*rt).second;
			m_pRoom->AddActor(this);
			return;
		}
	}
}

// Die - Handle death and respawning
void Actor::Die(Actor * pKiller, bool bSilent)
{
	if (m_nType == _NPC)
	{
		if (!bSilent)
			DeathCry(pKiller);

		// Will create a corpse and ensure the correct items are added to it/dropped
		CreateCorpse();

		// Remove the dead NPC from the room and release the memory
		m_pRoom->RemActor(this);
		delete this;
		return;
	}
	else
	{
		// Death determined by the GameWorld config settings
		// 1 = Happyland, no penalties, respawn in a random location
		// 2 = Default, Death permanently subtracts one point from nominated attribute id, having no more of this attribute results in permadeath
		// 3 = Final, Death is final, when you are killed are you dead
		int nType = m_pWorld->m_Variables->VarI("death_type");
		string sAtt  = m_pWorld->m_Variables->Var("death_attribute");	// attr_#
		bool bDeath = false;	// Perma death?

		if (nType == 1)
		{
			if (!bSilent)
				DeathCry(pKiller, true);

			CreateCorpse(true);
			Respawn();
			return;
		}
		else if (nType == 2)
		{
			if (AttrMax(sAtt) <= 1)
				bDeath = true;
			else
			{
				string sCurrent = "curr_";
				string sMax 	= "max_";
				sCurrent.append(sAtt);	// curr_attr_1
				sMax.append(sAtt);		// max_attr_1

				// Send a defeat message
				if (!bSilent)
					DeathCry(pKiller, true);
				// Drop items
				CreateCorpse(true);
				// Decrease their attribute current and max
				m_Variables->AddToVar(sCurrent, -1);
				m_Variables->AddToVar(sMax, -1);
				// Save changes
				((Player*)this)->Save(sCurrent, m_Variables->VarI(sCurrent));
				// Respawn elsewhere
				Respawn();
				return;
			}
		}
		else
			bDeath = true;


		if (bDeath)
		{
			// Give a message
			if (!bSilent)
				DeathCry(pKiller);
			Send("\n\rReturning to Account Menu\n\r");
			// Create a corpse
			CreateCorpse();
			this->AddFlag("delete");
			return;
		}

	}
}

// Death Cry - Display a message upon death
void Actor::DeathCry(Actor * pKiller, bool bDefeat)
{
	if (m_nType == _NPC)
		m_pRoom->Send(this, "%s falls to the ground, DEAD!\n\r", m_sName.c_str());
	else
	{
		string sCMsg = m_pWorld->m_Variables->Var("death_ch_msg");
		string sMsg = m_pWorld->m_Variables->Var("death_msg");
		string sKiller = m_pWorld->m_Variables->Var("death_vi_msg");
		string sScope = m_pWorld->m_Variables->Var("death_m_scope");

		if (bDefeat)
		{
			sCMsg = m_pWorld->m_Variables->Var("defeat_ch_msg");
			sMsg = m_pWorld->m_Variables->Var("defeat_msg");
			sKiller = m_pWorld->m_Variables->Var("defeat_vi_msg");
		}

		// Parse the string to accept the act format
		sCMsg = ActReplace(ACT_CH, this, pKiller, NULL, NULL, sCMsg, this);
		sKiller = ActReplace(ACT_VICTIM, this, pKiller, NULL, NULL, sKiller, pKiller);
		sMsg = ActReplace(ACT_ROOM, this, pKiller, NULL, NULL, sMsg, this);

		Send("%s\n\r", sCMsg.c_str());

		pKiller->Send("%s\n\r", sKiller.c_str());

		if (sScope == "Local")
			m_pRoom->Send(this, "%s\n\r", sMsg.c_str());
		else if (sScope == "Global")
			m_pWorld->Send(this, "%s", sMsg.c_str());
	}
	return;
}

// Determine what loot is dropped
void Actor::DetermineLoot()
{
	// The loot table is a map that maps an Item ID to the chance of it proccing
	// For Example: Item [Rusty Sword] Chance of dropping 50%
	// For this to drop you would have to roll a D100 and score less than or equal to 50
	// This gets slightly more complicated for NPCs with larger loot tables
	// For Example: Item [Rusty Sword]   Chance 60%
	//              Item [Lamella Armor] Chance 40%

	Item * pLoot = new Item;
	int nItemId = 0;

	int nTotal = 0;
	int nCount = 0;

	if (m_Loot.size() > 0)
	{
		for (map<int,int>::iterator it = m_Loot.begin(); it != m_Loot.end(); it++)
			nTotal += (*it).second;

		int nRandom = Main::rand_range(0, nTotal);

		for (map<int,int>::iterator it = m_Loot.begin(); it != m_Loot.end(); it++)
		{
			nCount += (*it).second;

			if (nRandom <= nCount)
			{
				nItemId = (*it).first;
				break;
			}
		}

		pLoot->m_nId = nItemId;
		pLoot->Load();
		m_pRoom->m_Items.push_back(pLoot);
		m_pRoom->Send("%s falls to the ground.\n\r", pLoot->m_sName.c_str());
		return;
	}

	return;
}

// Create Corpse - Create a corpse for an actor and fill it full
// The contents of the corpse are determined by the Game Config file
// Field: death_corpse
// Options: Inventory	- All items in inventory added to corpse
//			Equipment	- All worn items added to corpse
//          Both		- All added to corpse
// Will also handle death_drop for dropping of items based on options, uses the same as above
void Actor::CreateCorpse(bool bDefeat)
{
	stringstream ssStr;
	ssStr << "Corpse of " << m_sName;

	string sCorpse = m_pWorld->m_Variables->Var("death_corpse");
	string sDrop   = m_pWorld->m_Variables->Var("death_drop");

	// Create the corpse
	Item * pItem = new Item;
	pItem->m_sName = ssStr.str();
	pItem->m_Variables->Set("type", "corpse");
	pItem->AddFlag("No Get");
	pItem->m_Variables->Set("decay_timer", 600);	// Lasts 10 minutes
	pItem->m_Variables->Set("desc", "A corpse lies here, slowly starting to develop a rather unique stench.");

	// Worn items fall onto the ground
	for (map<string,Item*>::iterator it = m_Equipment.begin(); it != m_Equipment.end(); it++)
	{
		if (bDefeat && (sDrop == "Both" || sDrop == "Equipment"))
		{
			if (m_pRoom)
				m_pRoom->m_Items.push_back((*it).second);
		}
		else if (sCorpse == "Both" || sCorpse == "Equipment")
			pItem->m_Contents.push_back((*it).second);
	}

	for (list<Item*>::iterator it = m_Inventory.begin(); it != m_Inventory.end(); it ++)
	{
		if (bDefeat && (sDrop == "Both" || sDrop == "Inventory"))
		{
			if (m_pRoom)
				m_pRoom->m_Items.push_back((*it));
		}
		else if (sCorpse == "Both" || sCorpse == "Inventory")
			pItem->m_Contents.push_back((*it));
	}

	if (bDefeat)
	{
		delete pItem;
		pItem = NULL;
	}
	else
	{
		// Add the item to the room
		if (m_pRoom)
			m_pRoom->m_Items.push_back(pItem);
	}
}

// Update handler
void Actor::Update()
{
	if (m_nType == _PLAYER)
	{
		m_Variables->AddToVar("curr_timer", PULSE_ACTOR);
		m_Variables->AddToVar("max_timer", PULSE_ACTOR);

		((Player*)this)->Save("max_timer", m_Variables->VarI("max_timer"));
	}

	// Update any affects applied to this character
	list<string> DeleteAffects;

	// Reduce timers on
	for (map <string, Affect*>::iterator it = m_Affects.begin(); it != m_Affects.end(); it++)
	{
		(*it).second->m_nSeconds -= PULSE_ACTOR;

		// Update focus every 10 seconds
		if ((*it).first == "focus" && (*it).second->m_nSeconds % 10 == 0)
		{
			(*it).second->m_nModifier--;	// You lose a point of focus every 10 seconds
		}

		if ((*it).second->m_nSeconds <= 0 && !(*it).second->m_bPermanent)
		{
			// Handle meditation
			// Reset the timer to 6 seconds and away they go!
			if ((*it).first == "meditation")
			{
				// Reset the timer
				(*it).second->m_nSeconds = 6;
				int nFactor = 1;

				if (m_pRoom->HasFlag("holy"))
					nFactor = 2;

				// Increase their spirit by one if they can
				if (m_Variables->VarI("curr_attr_5") < m_Variables->VarI("max_attr_5"))
				{
					m_Variables->AddToVar("curr_attr_5", 1 * nFactor);	// Increase by 1 (or 2 if in a holy room)

					switch (Main::rand_range(1, 5))
					{
					case 1: Send("&GYou feel one with your surroundings, deep in meditation.\n\r");
						break;

					case 2: Send("&GDeep in meditation, you feel your soul healing.\n\r");
						break;

					case 3: Send("&GIn meditation nothing else matters...\n\r");
						break;

					case 4: Send("&GOhhhhhhhhhhhhhhhmmmmmmmmmm!\n\r");
						break;

					case 5: Send("&GYou are... meditating?!?\n\r");
						break;
					}
				}

				bool bMax = true;

				if (m_Variables->VarI("curr_hp") != m_Variables->VarI("max_hp"))
					bMax = false;

				// Heal some Str + 6 HP
				Heal((Attribute("attr_1") + 6) * nFactor);

				// Check for removal or lessening of bleed affects
				float fHp = 100.0 * ( float(m_Variables->VarI("curr_hp")) / float(m_Variables->VarI("max_hp")) );

				if (fHp >= 66.0 && IsAffected("bleeding"))
				{
					RemAffect("bleeding");
					Act("$n manage$p to stop bleeding.");
				}

				if (fHp >= 33.0 && IsAffected("bleeding heavily"))
				{
					RemAffect("bleeding heavily");
					AddAffect("bleeding");
					Act("$n manage$p to slow the rate of bloodloss.");
				}

				if ((m_Variables->VarI("curr_hp") == m_Variables->VarI("max_hp")) && !bMax)
					Send("&GYou feel fully healed!\n\r");
			}
			else
			{
				// Message disabled at Ken's request
				// Send("%s has expired.\n\r", (*it).first.c_str());
				DeleteAffects.push_back((*it).first);
			}
		}
	}

	for (list<string>::iterator it = DeleteAffects.begin(); it != DeleteAffects.end(); it ++)
	{
		RemAffect((*it));
	}

}

// Affect functions
void Actor::AddAffect(string sAff)
{
	if (!IsAffected(sAff))
	{
		Affect * pAffect = new Affect;
		pAffect->m_sName = sAff;
		pAffect->m_nLocation = AFF_NONE;
		pAffect->m_nModifier = 0;
		pAffect->m_nSeconds = -1;
		pAffect->m_bPermanent = true;
		m_Affects[sAff] = pAffect;
	}
}

void Actor::AddAffect(string sAff, int nLocation, int nModifier, int nM, int nS)
{
	if (!IsAffected(sAff))
	{
		Affect * pAffect = new Affect;
		pAffect->m_sName = sAff;
		pAffect->m_nLocation = nLocation;
		pAffect->m_nModifier = nModifier;
		if (nS == -1)
			pAffect->m_bPermanent = true;

		pAffect->m_nSeconds = (nM * 60) + nS;
		m_Affects[sAff] = pAffect;
	}
}

void Actor::AddAffect(string sAff, int nM, int nS)
{
	if (!IsAffected(sAff))
	{
		Affect * pAffect = new Affect;
		pAffect->m_sName = sAff;
		pAffect->m_nLocation = AFF_NONE;
		if (nS == -1)
			pAffect->m_bPermanent = true;
		pAffect->m_nSeconds = (nM * 60) + nS;
		m_Affects[sAff] = pAffect;
	}
}

void Actor::RemAffect(string sAff)
{
	if (IsAffected(sAff))
	{
		Affect * pAffect = m_Affects[sAff];
		m_Affects.erase(sAff);
		delete pAffect;
	}
}

void Actor::IncreaseAffect(string sAff, int nMod)
{
	if (IsAffected(sAff))
	{
		Affect * pAffect = m_Affects[sAff];
		pAffect->m_nModifier += nMod;
		return;
	}

	return;
}

void Actor::IncreaseTime(string sAff, int nM, int nS)
{
	if (IsAffected(sAff))
	{
		Affect * pAffect = m_Affects[sAff];
		pAffect->m_nSeconds += (nS + (nM * 60));
		return;
	}
}

bool Actor::IsAffected (string sAff)
{
	if (m_Affects.find(sAff) != m_Affects.end())
		return true;

	return false;
}

int Actor::AffectMod(string sAff)
{
	if (!IsAffected(sAff))
		return 0;

	return m_Affects[sAff]->m_nModifier;
}

// Attribute handlers
// For Shinobi valid attributes are 1=Strength, 2=Agility, 3=Wisdom, 4=Cunning, 5=Spirit
// This is defined in the XML world configuration file.
// Attributes can be modified by affects hence this function needs to return the modified current
// value for an Attribute. The current (curr_) and maximum values (max_) are stored as Variables for
// the Actors. The current variable is the unmodified value, it can only be reduced through usage i.e.
// expending spirit to hide. The maximum value is the current maximum value of an attribute for an Actor.
// This can be increased through training the stat.
int Actor::Attribute(string sA)
{
	string sAttr = "curr_";
	sAttr.append(sA);
	int nLocation = AFF_NONE;

	if (sA == "attr_1")
		nLocation = AFF_ATTR_1;
	else if (sA == "attr_2")
		nLocation = AFF_ATTR_2;
	else if (sA == "attr_3")
		nLocation = AFF_ATTR_3;
	else if (sA == "attr_4")
		nLocation = AFF_ATTR_4;
	else if (sA == "attr_5")
		nLocation = AFF_ATTR_5;

	if (m_Variables->HasVar(sAttr))
	{
		// Get the current value of the Attribute
		int nCurrent = m_Variables->VarI(sAttr);
		int nMod = 0;

		// Now check for any modifications to this by Affects
		for (map<string, Affect*>::iterator it = m_Affects.begin(); it != m_Affects.end(); it++)
		{
			// Check for any affecs that affect this stat
			if ((*it).second->m_nLocation == nLocation)
				nMod += (*it).second->m_nModifier;
		}

		return nCurrent + nMod;
	}
	else
		return 0;
}

int Actor::AttrMax (string nAttr)
{
	string sAttr = "max_";
	sAttr.append(nAttr);

	if (m_Variables->HasVar(sAttr))
	{
		return m_Variables->VarI(sAttr);
	}
	else
		return 0;
}
