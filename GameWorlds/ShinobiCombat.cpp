/*
 * ShinobiCombat.cpp
 *
 *  Created on: Sep 19, 2012
 *      Author: owen
 */

#include "ShinobiCombat.h"
#include "../Combat.h"
#include "../Actor.h"
#include "../Areas/Item.h"
#include "../Areas/Exit.h"
#include "../Main.h"

using namespace std;

ShinobiCombat::ShinobiCombat()
{
	// TODO Auto-generated constructor stub
	m_nDamage = 0;
	m_bEnded = false;
	m_nRounds = 0;

	// Define valid moves in combat
	m_Moves.push_back("strike");
	m_Moves.push_back("surge");
	m_Moves.push_back("block");
	m_Moves.push_back("dodge");
	m_Moves.push_back("feint");
	m_Moves.push_back("flee");
}

ShinobiCombat::~ShinobiCombat()
{
	// TODO Auto-generated destructor stub
}

void ShinobiCombat::End()
{
	Reset(m_pAttacker);
	Reset(m_pDefender);
	m_pAttacker->m_pCombat = NULL;
	m_pDefender->m_pCombat = NULL;
	m_pAttacker->m_pTarget = NULL;
	m_pDefender->m_pTarget = NULL;

	// Remove combat affects
	list<string> CombatAffects;

	CombatAffects.push_back("dazed");
	CombatAffects.push_back("staggered");
	CombatAffects.push_back("prone");
	CombatAffects.push_back("overextended");
	CombatAffects.push_back("cornered");

	for (list<string>::iterator it = CombatAffects.begin(); it != CombatAffects.end(); it++)
	{
		if (m_pAttacker->IsAffected(*it))
			m_pAttacker->RemAffect(*it);

		if (m_pDefender->IsAffected(*it))
			m_pDefender->RemAffect(*it);
	}

	m_bEnded = true;
}

// Shinobi Combat -- ANCHILLARY FUNCTIONS

// FUMBLE -- Do nothing!
void Fumble (Actor * pAttacker, Actor * pVictim)
{
	pAttacker->m_pWorld->SendCombatMsg(pAttacker, pVictim, "fumble");
	// Reset Victim's rate
	pAttacker->m_Variables->Set("curr_rate", 0);
	return;
}

// BLOCK -- Defensive, attempt to prevent a strike
void Block(Actor * pAttacker, Actor * pVictim)
{
	// Reset Victim's rate
	pVictim->m_Variables->Set("curr_rate", 0);
	return;
}

// DODGE -- Defensive, attempt to avoid an attack
// In this function the VICTIM is the player using DODGE to try to prevent
// them being hit by a STRIKE from the attacker
bool Dodge (Actor * pVictim, Actor * pAttacker)
{
	// Reset Victim's rate
	pVictim->m_Variables->Set("curr_rate", 0);

	int nAtt = pAttacker->Attribute("attr_2");
	int nVic = pVictim->Attribute("attr_2");

	if (pAttacker->HasFlag("initiative"))
		nAtt++;
	if (pVictim->HasFlag("initiative"))
		nVic++;

	if (pVictim->HasFlag("power"))
		nVic += 5;
	else if (pVictim->HasFlag("super"))
		nVic += 10;

	if (nVic >= nAtt)
		return true;

	return false;
}

// Actual deal damage to players, give messages, damage equipment, consume rof, etc
void ComputeStrike(Actor * pAttacker, Actor * pVictim, bool bFree = false)
{
	int nDam = pAttacker->m_pCombat->m_nDamage;

	// Handle the after effects of the strike
	// First check the strike actually caused some damage!
	if (nDam > 0)
	{
		// Generate a message for the attack
		if (pAttacker->HasFlag("power"))
		{
			pAttacker->m_pWorld->SendCombatMsg(pAttacker, pVictim, "strike", "power");
			pAttacker->RemFlag("power");
		}
		else if (pAttacker->HasFlag("super"))
		{
			pAttacker->m_pWorld->SendCombatMsg(pAttacker, pVictim, "strike", "super");
			pAttacker->RemFlag("super");
		}
		else
			pAttacker->m_pWorld->SendCombatMsg(pAttacker, pVictim, "strike", "");

		// [1] Deal Damage
		pVictim->Damage(pAttacker->m_pCombat->m_nDamage);

		// [2] Reduce Weapon quality (1% chance)
		if (Main::rand_range(1, 100) == 1)
			pVictim->DamageItem("hands", 1);

		if (pAttacker->m_pCombat->m_bResist)
		{
			pAttacker->m_pWorld->SendCombatMsg(pVictim, pAttacker, "strike", "resist");
			// [3] Reduce Armour quality
			pVictim->DamageItem("body", 1);
		}

		// The Victim may now be dead, we need to check this
		if (pVictim->m_Variables->VarI("curr_hp") <= 0)
		{
			pVictim->Die(pAttacker);
			pAttacker->m_pCombat->End();
		}
	}
}

// STRIKE -- Calculate the results of one player hitting another and save it
// to the COMBAT object for later computation
void CalculateStrike(Actor * pAttacker, Actor * pVictim, bool bFree)
{
	// Check weapon being used has available rate of fire
	Item * pWeapon = pAttacker->GetEquipped("hands");

	if (pWeapon && pWeapon->HasFlag("weapon"))
	{
		// If they have used strike more than RATE times in a row they fumble!
		if (pAttacker->m_Variables->VarI("curr_rate") >= pWeapon->m_Variables->VarI("weapon_rate"))
		{
			pAttacker->m_pCombat->m_nDamage = STRIKE_FUMBLE;
			return;
		}
		else
		{
			// Increment their rate by one
			pAttacker->m_Variables->AddToVar("curr_rate", 1);
		}
	}

	// Move onto hit calculations
	float fBase = 0.0;

	// Set the base chance to hit
	if (pAttacker->m_Variables->VarI("stance") == COMBAT_DEFENSIVE)
		fBase = 65.0;	// 65% base hit in Defensive
	else
		fBase = 95.0;	// 95% base hit in Offensive

	if (pAttacker->m_pWorld->m_Variables->VarI("combat_debug") == 1)
		pAttacker->Send("Base(%0.0f)", fBase);

	// ACCURACY: Add weapon quality if its a sword
	if (pWeapon && pWeapon->HasFlag("sword"))
	{
		if (pAttacker->m_pWorld->m_Variables->VarI("combat_debug") == 1)
			pAttacker->Send(" +SW(%d)", pWeapon->m_Variables->VarI("curr_quality"));

		fBase += (float)pWeapon->m_Variables->VarI("curr_quality");
	}

	// Subtract targets AGILITY (attr_2) * 2
	if (pAttacker->m_pWorld->m_Variables->VarI("combat_debug") == 1)
		pAttacker->Send(" -AG(%d)", 2*pVictim->Attribute("attr_2"));

	fBase -= float(2 * pVictim->Attribute("attr_2"));

	// Subtract -10% if VICTIM has initiative
	if (pVictim->HasFlag("initiative"))
	{
		if (pAttacker->m_pWorld->m_Variables->VarI("combat_debug") == 1)
			pAttacker->Send(" -IN(10)");

		fBase -= 10.0;
	}

	// Subtract -20% if VICTIM is surging
	if (pVictim->m_Variables->Var("combat_move") == "surge")
	{
		if (pAttacker->m_pWorld->m_Variables->VarI("combat_debug") == 1)
			pAttacker->Send(" -SG(20)");

		fBase -= 20.0;
	}

	// Deal with VICTIMs that are BLOCKING, Free Strikes ignore this rule
	if (pVictim->m_Variables->Var("combat_move") == "block" && !bFree)
	{
		int nRed = 0;
		// Cannot block a SUPER STRIKE without at LEAST a POWER BLOCK
		if (pAttacker->HasFlag("super"))
		{
			if (pVictim->HasFlag("power") || pVictim->HasFlag("super"))
			{
				// Victim gains block bonus
				if (pVictim->HasFlag("power"))
					nRed = 80;
				else if (pVictim->HasFlag("super"))
					nRed = 100;
			}
		}
		else
		{
			nRed = 50;
		}

		if (pAttacker->m_pWorld->m_Variables->VarI("combat_debug") == 1)
			pAttacker->Send(" -BK(%d)", nRed);

		// Set the block bool to true to record the fact that the victim blocked
		// some or all of the attack
		if (nRed > 0)
			pAttacker->m_pCombat->m_bBlock = true;

		fBase -= (float) nRed;
	}

	// Subtract 20% if victim is in a defensive stance
	if (pVictim->m_Variables->VarI("combat_stance") == COMBAT_DEFENSIVE)
	{
		if (pAttacker->m_pWorld->m_Variables->VarI("combat_debug") == 1)
			pAttacker->Send(" -DEF(-20)");

		fBase -= 20.0;
	}

	// Multiply by 0.5x if Victim is performing a FLEE
	if (pVictim->m_Variables->Var("combat_move") == "flee")
	{
		if (pAttacker->m_pWorld->m_Variables->VarI("combat_debug") == 1)
			pAttacker->Send(" *FL(0.5)");

		fBase *= 0.5;
	}

	// Multiple by 0.5x if Attacker is prone
	if (pAttacker->IsAffected("prone"))
	{
		if (pAttacker->m_pWorld->m_Variables->VarI("combat_debug") == 1)
			pAttacker->Send(" *PR(0.5)");

		fBase *= 0.5;
	}

	int nD100 = Main::rand_range(1, 100);

	if (pAttacker->m_pWorld->m_Variables->VarI("combat_debug") == 1)
		pAttacker->Send("== %0.0f D100(%d)\n\r", fBase, nD100);

	int nBase = (int) fBase;

	if (nBase > 95)
		nBase = 95;

	if (nBase < 0)
		nBase = 5;

	// Roll less than a hit chance on a D100 to hit the target
	if (nD100 <= nBase)
	{
		// Attack HIT, determine DAMAGE
		// 20 + 2 * (Strength + Weapon Quality)
		float nDam = pAttacker->Attribute("attr_1");

		if (pWeapon)
			nDam += float(pWeapon->m_Variables->VarI("curr_quality"));

		nDam *= 2.0;

		// Base damage is determined by the Game World
		if (pAttacker->m_pWorld->m_Variables->VarI("combat_base_damage") > 0)
			nDam += pAttacker->m_pWorld->m_Variables->VarI("combat_base_damage");
		else
			nDam += 20.0;


		if (pAttacker->m_pWorld->m_Variables->VarI("combat_debug") == 1)
			pAttacker->Send("Dam (%0.0f)", nDam);

		// Increase damage by 10% if attacker has poison
		if (pAttacker->m_pWorld->FindItem("poison", pAttacker))
		{
			if (pAttacker->m_pWorld->m_Variables->VarI("combat_debug") == 1)
				pAttacker->Send(" *POI(1.1)");

			nDam *= 1.1;
		}

		// Double damage for POWER, Triple for SUPER
		if (pAttacker->HasFlag("power") && !bFree)
		{
			if (pAttacker->m_pWorld->m_Variables->VarI("combat_debug") == 1)
				pAttacker->Send(" *PWR(2.0)");

			nDam *= 2.0;
		}
		else if (pAttacker->HasFlag("super") && !bFree)
		{
			if (pAttacker->m_pWorld->m_Variables->VarI("combat_debug") == 1)
				pAttacker->Send(" *SUP(3.0)");

			nDam *= 3.0;
		}

		if (pAttacker->m_pWorld->m_Variables->VarI("combat_debug") == 1)
			pAttacker->Send("== %0.0f", nDam);

		// RESIST Damage
		// Base chance is (Str + Armour)%
		int nRes = pVictim->Attribute("attr_1") + pVictim->Armour("curr_quality");

		if (nRes < 0)
			nRes = 1;
		else if (nRes > 95)
			nRes = 95;

		nD100 = Main::rand_range(1, 100);

		// To resist an attack we have to roll a D100 and score less our resistance rating
		if (Main::rand_range(1, 100) <= nRes)
		{
			if (pAttacker->m_pWorld->m_Variables->VarI("combat_debug") == 1)
				pAttacker->Send(" (Resisted: %d vs D100(%d))", nRes, nD100);

			// Reduce damage by dividing by 10
			nDam /= 10.0;

			// Mark the strike as resisted
			pAttacker->m_pCombat->m_bResist = true;
		}

		if (pAttacker->m_pWorld->m_Variables->VarI("combat_debug") == 1)
			pAttacker->Send("\n\r");

		// Strike is complete, set the damage done and return
		pAttacker->m_pCombat->m_nDamage = nDam;

	}
	else
	{
		pAttacker->m_pCombat->m_nDamage = STRIKE_MISS;
	}


	return;
}

// STRIKE -- One player hitting another player
// Its broken down into Calculate and Compute to allow for Simultaneous Strikes, to maximise code
// reusability
void Strike (Actor * pAttacker, Actor * pVictim, bool bFree)
{
	// Crunch the numbers
	CalculateStrike (pAttacker, pVictim, bFree);
	// Do the damage
	int nDam = pAttacker->m_pCombat->m_nDamage;

	if (nDam > 0)
	{
		// Did some damage so we compute the strike which also handles the combat message
		ComputeStrike(pAttacker, pVictim, bFree);
	}
	else if (nDam == STRIKE_FUMBLE)
	{
		// Free strikes do not give a message
		if (!bFree)
			Fumble(pAttacker, pVictim);
	}
	else if (nDam == STRIKE_MISS)
	{
		if (pAttacker->m_pCombat->m_bBlock)
		{
			// Weapon takes damage on a 1% chance
			if (pVictim->GetWeapon() && Main::rand_range(1, 100) == 1)
				pVictim->DamageItem("hands", 1);

			// Expend any surges that were used here
			if (pVictim->HasFlag("power"))
			{
				pAttacker->m_pWorld->SendCombatMsg(pVictim, pAttacker, "block", "power");
				pVictim->RemFlag("power");
			}
			else if (pVictim->HasFlag("super"))
			{
				pAttacker->m_pWorld->SendCombatMsg(pVictim, pAttacker, "block", "super");
				pVictim->RemFlag("super");
			}
			else
				pAttacker->m_pWorld->SendCombatMsg(pVictim, pAttacker, "block", "");
		}
		else
		{
			pAttacker->m_pWorld->SendCombatMsg(pAttacker, pVictim, "strike", "miss");
		}
	}

	// Reset Strike variables
	if (pAttacker->m_pCombat)
		pAttacker->m_pCombat->ResetStrike();

	return;
}

void SimStrike(Actor * pAttacker, Actor * pVictim)
{
	// For a simultaneous strike potentially both players could kill each other!
	// Damage taken, Whether they resisted or blocked is stored in the variables below
	// Array index 0 is used for the results of pAttacker striking pVictim
	// Array index 1 is used for the results of pVictim striking pAttacker
	int nAttacker = 0;
	int nVictim = 1;

	int  nDam[2];
	bool bRes[2];
	bool bBlo[2];
	Actor * pAct[2];

	pAct[nAttacker] = pAttacker;
	pAct[nVictim] = pVictim;

	// Calculate the results of the first strike
	CalculateStrike(pAttacker, pVictim, false);
	nDam[nAttacker] = pAttacker->m_pCombat->m_nDamage;
	bRes[nAttacker] = pAttacker->m_pCombat->m_bResist;
	bBlo[nAttacker] = pAttacker->m_pCombat->m_bBlock;
	pAttacker->m_pCombat->ResetStrike();

	// Calculate results of the second strike
	CalculateStrike(pAttacker, pVictim, false);
	nDam[nVictim] = pAttacker->m_pCombat->m_nDamage;
	bRes[nVictim] = pAttacker->m_pCombat->m_bResist;
	bBlo[nVictim] = pAttacker->m_pCombat->m_bBlock;
	pAttacker->m_pCombat->ResetStrike();

	// Handle the simultaneous strike here
	bool bFirstStrike[2] = { false, false };

	// [1] Check for First Strike flag
	if (pAttacker->GetWeapon() && pAttacker->GetWeapon()->HasFlag("first strike"))
		bFirstStrike[nAttacker] = true;

	if (pVictim->GetWeapon() && pVictim->GetWeapon()->HasFlag("first strike"))
		bFirstStrike[nVictim] = true;

	// If they both have First Strike or neither have First Strike it remains a sim strike
	if ((bFirstStrike[nAttacker] && bFirstStrike[nVictim]) || (!bFirstStrike[nAttacker] && !bFirstStrike[nVictim]))
	{
		//  Owen and Ken strike each other simultaneously!
		//   Owen hits for X damage
		//   Ken hits for Y damage, killing Owen!
		pAttacker->m_pWorld->SendCombatMsg(pAttacker, pVictim, "strike", "simultaneous");

		int nOther = 1;

		for (int i = 0; i < 2; i++)
		{
			if (nDam[i] > 0)
			{
				if (bBlo[i])
				{
					// Weapon damaged as used to block
					if (pAct[nOther]->GetWeapon())
						pAct[nOther]->DamageItem("hands", 1);

					// Did $N block the strike?
					pAttacker->m_pWorld->SendCombatMsg(pAct[nOther], pAct[i], "block");
				}
				else
				{
					// Deal them damage
					pAct[nOther]->Damage(nDam[i]);

					// Reduce weapon quality (1% chance)
					if (Main::rand_range(1, 100) == 1)
						pAct[i]->DamageItem("hands", 1);

					// Did $N's armour absorb some of the strike?
					if (bRes[i])
					{
						pAttacker->m_pWorld->SendCombatMsg(pAct[i], pAct[nOther], "strike");
						pAttacker->m_pWorld->SendCombatMsg(pAct[nOther], pAct[i], "strike", "resist");
						pAct[nOther]->DamageItem("body", 1);
					}
					else
						pAttacker->m_pWorld->SendCombatMsg(pAct[i], pAct[nOther], "strike");

					// Check for death
					if (pAct[nOther]->m_Variables->VarI("curr_hp") <= 0)
					{
						//pAct[i]->Act("&G", "&R", pAct[nOther], NULL, NULL, "$N $A dead!");
						pAct[nOther]->Die(pAct[i], false);
						pAct[i]->m_pCombat->End();
						return;
					}
				}
			}
			else
			{
				pAttacker->m_pWorld->SendCombatMsg(pAct[i], pAct[nOther], "strike", "miss");
			}

			nOther--;
		}


	}
	else
	{
		int nFirst = 0;
		int nSecon = 0;
		Actor * pFirst = NULL;
		Actor * pSecon = NULL;

		// One has the advantage and strikes first!
		if (bFirstStrike[nAttacker])
		{
			pFirst = pAttacker;
			nFirst = nAttacker;
			pSecon = pVictim;
			nSecon = nVictim;
		}
		else
		{
			pFirst = pVictim;
			nFirst = nVictim;
			pSecon = pAttacker;
			nSecon = nAttacker;
		}

		// Complete each attack in turn
		pAttacker->m_pCombat->m_nDamage = nDam[nFirst];
		pAttacker->m_pCombat->m_bResist = bRes[nFirst];
		pAttacker->m_pCombat->m_bBlock  = bBlo[nFirst];
		ComputeStrike(pFirst, pSecon);

		// Check the strike didn't kill the target
		if (pSecon->m_Variables->VarI("curr_hp") <= 0)
		{
			pSecon->Die(pFirst, false);
			pSecon->m_pCombat->End();
		}

		pAttacker->m_pCombat->m_nDamage = nDam[nSecon];
		pAttacker->m_pCombat->m_bResist = bRes[nSecon];
		pAttacker->m_pCombat->m_bBlock  = bBlo[nSecon];
		ComputeStrike(pSecon, pFirst);

		// Check the strike didn't kill the target
		if (pFirst->m_Variables->VarI("curr_hp") <= 0)
		{
			pFirst->Die(pSecon, false);
			pFirst->m_pCombat->End();
			return;
		}

	}

	// End of round
	pAttacker->m_pCombat->Reset(pAttacker);
	pAttacker->m_pCombat->Reset(pVictim);
	return;
}

// Combat Stunt -- Perform an action
void CombatStunt (Actor * pAttacker, Actor * pVictim)
{
	// STUNT List
	// [1] DISARM
	// [2] FREE STRIKE
	// [3] FREE SURGE
	// [4] STAGGERED
	// [5] DAZED
	// [6] PRONE
	// [7] OVEREXTENDED
	// [8] CORNERED
	// [9] BREAK POISON

	vector<string> StuntList;

	// To determine the combat stunt we will add each VALID stunt to a list and then randomly
	// determine a stunt from the list, this function will pick a random stunt and then carry out
	// its actions

	// [1] DISARM - Check they have a weapon
	if (pVictim->GetWeapon())
		StuntList.push_back("disarm");

	// [2] FREE STRIKE - Check they have a free unit of ROF
	if (!pAttacker->GetWeapon() || (pAttacker->GetWeapon() && pAttacker->m_Variables->VarI("curr_rate") < pAttacker->Weapon("weapon_rate")))
		StuntList.push_back("free strike");

	// [3] FREE SURGE - Check they are not currently surged
	if (!pAttacker->HasFlag("power") && !pAttacker->HasFlag("super"))
		StuntList.push_back("free surge");

	// [4] STAGGERED - Check they aren't staggered
	if (!pVictim->IsAffected("staggered"))
		StuntList.push_back("staggered");

	// [5] DAZED - Check not dazed already
	if (!pVictim->IsAffected("dazed"))
		StuntList.push_back("dazed");

	// [6] PRONE - Check not prone already
	if (!pVictim->IsAffected("prone"))
		StuntList.push_back("prone");

	// [7] OVEREXTENDED - Check not overextended already
	if (!pVictim->IsAffected("overextended"))
		StuntList.push_back("overextended");

	// [8] Cornered - Check not cornered already
	if (!pVictim->IsAffected("cornered"))
		StuntList.push_back("cornered");

	// [9] Break Poison - Check they have poison
	if (pVictim->m_pWorld->FindItem("poision", pVictim))
		StuntList.push_back("break poison");

	// Select a random stunt
	string sStunt = StuntList.at(Main::rand_range(0, StuntList.size()));

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// DISARM :: If the enemy is holding a weapon, he aint any more!
	///////////////////////////////////////////////////////////////////////////////////////////////////
	if (sStunt == "disarm")
	{
		Item * pItem = pVictim->GetWeapon();
		pVictim->m_Equipment["hands"] = NULL;
		pVictim->m_pRoom->m_Items.push_back(pItem);
		pAttacker->m_pWorld->SendCombatMsg(pAttacker, pVictim, "disarm");
		return;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// FREE STRIKE :: Hit them
	///////////////////////////////////////////////////////////////////////////////////////////////////
	if (sStunt == "free strike")
	{
		pAttacker->m_pWorld->SendCombatMsg(pAttacker, pVictim, "feint", "strike");
		Strike(pAttacker, pVictim, false);
		return;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// FREE SURGE :: Power up
	///////////////////////////////////////////////////////////////////////////////////////////////////
	if (sStunt == "free surge")
	{
		pAttacker->m_pWorld->SendCombatMsg(pAttacker, pVictim, "feint", "surge");
		pAttacker->AddFlag("power");
		pAttacker->Heal(10);
		pAttacker->m_pWorld->SendCombatMsg(pAttacker, pVictim, "surge", "power");
		return;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// STAGGERED :: Agility reduced by 50%
	///////////////////////////////////////////////////////////////////////////////////////////////////
	if (sStunt == "staggered")
	{
		int nAgi = pVictim->Attribute("attr_2");
		pAttacker->m_pWorld->SendCombatMsg(pAttacker, pVictim, "feint", "staggered");
		pVictim->AddAffect("staggered", AFF_ATTR_2, -(nAgi/2), 0, -1);
		return;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// DAZED :: Wisdom reduced by 50%, lose any surge
	///////////////////////////////////////////////////////////////////////////////////////////////////
	if (sStunt == "dazed")
	{
		int nWis = pVictim->Attribute("attr_3");
		pAttacker->m_pWorld->SendCombatMsg(pAttacker, pVictim, "feint", "dazed");
		pVictim->AddAffect("dazed", AFF_ATTR_3, -(nWis/2), 0, -1);
		return;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// PRONE :: 50% less accurate when Striking
	///////////////////////////////////////////////////////////////////////////////////////////////////
	if (sStunt == "prone")
	{
		pAttacker->m_pWorld->SendCombatMsg(pAttacker, pVictim, "feint", "prone");
		pVictim->AddAffect("prone");
		return;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// OVEREXTENDED :: Victim is unable to strike next round
	///////////////////////////////////////////////////////////////////////////////////////////////////
	if (sStunt == "overextended")
	{
		pAttacker->m_pWorld->SendCombatMsg(pAttacker, pVictim, "feint", "overextended");
		pVictim->AddAffect("overextended");
		return;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// CORNERED :: Victim unable to dodge next round
	///////////////////////////////////////////////////////////////////////////////////////////////////
	if (sStunt == "cornered")
	{
		pAttacker->m_pWorld->SendCombatMsg(pAttacker, pVictim, "feint", "cornered");
		pVictim->AddAffect("cornered");
		return;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// BREAK POISON :: Destroy a bottle of poison and hurt them
	///////////////////////////////////////////////////////////////////////////////////////////////////
	if (sStunt == "break poison")
	{
		Item * pItem = pVictim->m_pWorld->FindItem("poison", pVictim);

		// Sanity check
		if (!pItem)
			return;

		// Remove from Victim
		pVictim->m_Inventory.remove(pItem);
		// Give message
		pAttacker->m_pWorld->SendCombatMsg(pAttacker, pVictim, "feint", "poison");
		pVictim->Damage(Main::rand_range(10,30));

		if (pVictim->m_Variables->VarI("curr_hp") <= 0)
		{
			pAttacker->Act("&G", "&R", pVictim, NULL, NULL, "$N die$P covered in poison!");
			pAttacker->m_pCombat->End();
		}
		return;
	}

	return;
}

// FEINT -- Lure the attacker into making a mistake
void Feint (Actor * pAttacker, Actor * pVictim)
{
	// Reset rate
	pAttacker->m_Variables->Set("curr_rate", 0);

	int nVictimWisdom = pVictim->Attribute("attr_3");
	int nAttackerCunning = pAttacker->Attribute("attr_4");

	if (pAttacker->m_pWorld->m_Variables->VarI("combat_debug") == 1)
		pAttacker->Send("Feint: Attacker Cunning (%d) vs Victim Wisdom (%d)\n\r", nAttackerCunning, nVictimWisdom);

	// If the victim is SURGING then he gets to add his spirit to his wisdom in defense
	if (pVictim->m_Variables->Var("combat_move") == "surge")
		nVictimWisdom += pVictim->Attribute("attr_5");

	// If the victim is in a defensive stance he gets a +1 bonus
	if (pVictim->m_Variables->VarI("stance") == COMBAT_DEFENSIVE)
		nVictimWisdom++;

	// If either party is using a staff, they gain a +1 bonus
	if (pVictim->GetWeapon() && pVictim->GetWeapon()->HasFlag("staff"))
		nVictimWisdom++;

	if (pAttacker->GetWeapon() && pAttacker->GetWeapon()->HasFlag("staff"))
		nAttackerCunning++;

	// Whoever has the advantage gets a +1
	if (pAttacker->HasFlag("initiative"))
		nAttackerCunning++;

	if (pVictim->HasFlag("initiative"))
		nVictimWisdom++;

	if (pAttacker->m_pWorld->m_Variables->VarI("combat_debug") == 1)
		pAttacker->Send("               Modified (%d) vs Modified      (%d)\n\r", nAttackerCunning, nVictimWisdom);

	// Block does not require a roll to check for feint success
	if ((pVictim->m_Variables->Var("combat_move") == "block") || (nAttackerCunning > nVictimWisdom))
	{
		// Feint succesful
		CombatStunt(pAttacker, pVictim);
		return;
	}
	else
	{
		pAttacker->m_pWorld->SendCombatMsg(pAttacker, pVictim, "feint", "fail");
		//pAttacker->Act(pVictim, "$n fail$p to feint $N.");
	}


	return;
}

// SURGE -- Expend spirit to improve performance
// 1st Surge == POWER
// 2nd Surge == SUPER
// 3rd Surge == Essence Burn
// Costs a point of spirit, if none available its a fumble!
void Surge (Actor * pAttacker, Actor * pVictim)
{
	// Reset rate
	pAttacker->m_Variables->Set("curr_rate", 0);

	// If they have no more Spirit they fumble
	if (pAttacker->Attribute("attr_5") <= 0)
	{
		Fumble(pAttacker, pVictim);
		return;
	}

	// Drain a point of Spirit
	pAttacker->m_Variables->AddToVar("curr_attr_5", -1);

	// Remove Prone, Dazed or Staggered
	if (pAttacker->IsAffected("prone"))
		pAttacker->RemAffect("prone");
	if (pAttacker->IsAffected("dazed"))
		pAttacker->RemAffect("dazed");
	if (pAttacker->IsAffected("staggered"))
		pAttacker->RemAffect("staggered");

	// Add POWER or SUPER flag
	if (pAttacker->HasFlag("power"))
	{
		pAttacker->Heal(20);
		pAttacker->RemFlag("power");
		pAttacker->AddFlag("super");
		pAttacker->m_pWorld->SendCombatMsg(pAttacker, pVictim, "surge", "super");
	}
	else if (pAttacker->HasFlag("super"))
	{
		pAttacker->RemFlag("super");
		if (pAttacker->Damage(Main::rand_range(5, 35)))
		{
			pAttacker->m_pWorld->SendCombatMsg(pAttacker, pVictim, "surge", "death");
			pAttacker->Die(pAttacker, true);
			pAttacker->m_pCombat->End();
			return;
		}
		else
			pAttacker->m_pWorld->SendCombatMsg(pAttacker, pVictim, "surge", "burn");
	}
	else
	{
		pAttacker->AddFlag("power");
		pAttacker->Heal(10);
		pAttacker->m_pWorld->SendCombatMsg(pAttacker, pVictim, "surge", "power");
		//pAttacker->Act("&G", "&R", pVictim, NULL, NULL, "$n glint$p with stored energy!");
	}

	return;
}

// FLEE
// Attempt to escape from combat, if the player has a smokebomb in their inventory then the escape
// attempt is automatic
void Flee (Actor * pAttacker, Actor * pVictim)
{
	// First, set their stance to the aggressive stance
	pAttacker->m_Variables->Set("stance", COMBAT_OFFENSIVE);

	// Next check to see if the Actor has a smoke bomb
	Item * pItem = pAttacker->m_pWorld->FindItem("smokebomb", pAttacker);

	if (pItem)
	{
		pAttacker->m_pWorld->SendCombatMsg(pAttacker, pVictim, "flee", "smoke");
	}
	else
	{
		// No smokebomb so pVictim gets a free strike against pAttacker

		pAttacker->m_pWorld->SendCombatMsg(pAttacker, pVictim, "flee", "strike");
		Strike(pVictim, pAttacker, true);

		// Check for death
		if (pAttacker->m_Variables->VarI("curr_hp") <= 0)
			return;
		else
			pAttacker->m_pCombat->ResetStrike();
	}

	// Select a random exit and flee in that direction
	Exit * pExit = NULL;

	int nCount = Main::rand_range(0, pAttacker->m_pRoom->m_Exits.size());

	for (map<string, Exit*>::iterator it = pAttacker->m_pRoom->m_Exits.begin(); it != pAttacker->m_pRoom->m_Exits.end(); it++)
		if (nCount-- <= 0)
			pExit = (*it).second;

	if (pExit)
		pAttacker->Move(pExit->m_sDir);

	pAttacker->m_pCombat->End();

	return;
}

// Perform a single round's action for a Player
void CombatRound (Actor * pAttacker, Actor * pVictim)
{
	// CHECK MOVE
	if (!pAttacker->m_Variables->HasVar("combat_move"))
		Fumble(pAttacker, pVictim);

	// CONDUCT MOVE

	// STRIKE -- Try to whack a target
	if (StrEquals(pAttacker->m_Variables->Var("combat_move"), "strike"))
	{
		// Compute the Strike
		Strike (pAttacker, pVictim, false);

	}
	// SURGE -- Expend spirit to improve later moves
	else if (StrEquals(pAttacker->m_Variables->Var("combat_move"), "surge"))
	{
		Surge(pAttacker, pVictim);
		return;
	}
	// FEINT -- Try to trick the opponent
	else if (StrEquals(pAttacker->m_Variables->Var("combat_move"), "feint"))
	{
		if (StrEquals(pVictim->m_Variables->Var("combat_move") , "strike"))
			pAttacker->m_pWorld->SendCombatMsg(pAttacker, pVictim, "feint", "fail");
		else
			Feint(pAttacker, pVictim);

		return;
	}
	// FLEE - RUNAWAY!!!
	else if (StrEquals(pAttacker->m_Variables->Var("combat_move"), "flee"))
	{
		Flee(pAttacker, pVictim);
		return;
	}

	return;
}

// Update
// Update runs every 6 seconds and handles a single round of combat between two players.
void ShinobiCombat::Update()
{
	Actor * pFirst = NULL;
	Actor * pSecond = NULL;

	// First player to input a command gets to act first, this is known as taking the initiative
	if (m_pAttacker->m_Variables->VarI("combat_move_timer") >= m_pDefender->m_Variables->VarI("combat_move_timer"))
	{
		pFirst = m_pAttacker;
		pSecond = m_pDefender;
		pFirst->AddFlag("initiative");
	}
	else
	{
		pFirst = m_pDefender;
		pSecond = m_pAttacker;
		pSecond->AddFlag("initiative");
	}


	// pFirst  -- The FIRST player to act this round
	// pSecond -- No prizes for guessing what this one means!
	// The actual order players act in is only important for simultaneous strikes (i.e. when both
	// parties have decided to strike). Other than this instance all other actions are performed
	// at exactly the same time.

	// AMBUSH
	// If a player has a higher cunning attribute than their opponent there is a chance they are
	// performing an ambush attack. This counts as a free attack that happens only in the first
	// round and before any other attacks
	if (pFirst->m_pCombat->m_nRounds == 0)
	{
		if (pFirst->m_Variables->HasVar("combat_ambush"))
		{
			// Free strike
			pFirst->Act(pSecond, "$n ambush$z $N!");
			Strike(pFirst, pSecond, true);
			pFirst->m_Variables->RemVar("combat_ambush");
		}
		else if (pSecond->m_Variables->HasVar("combat_ambush"))
		{
			// Free strike
			pSecond->Act(pFirst, "$n ambush$z $N!");
			Strike(pSecond, pFirst, true);
			pSecond->m_Variables->RemVar("combat_ambush");
		}

		// Check the AMBUSH didn't kill either player
		if (pFirst->m_Variables->VarI("curr_hp") <= 0 || pSecond->m_Variables->VarI("curr_hp") <= 0)
		{
			End();
			return;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Combat Round time
	// Strikes must happen simultaneously and cater for the First Strike ability of the Spear
	// in order to handle this the Combat Round function should have special functionality for
	// strike commands, it should do ALL the mathematics and return the result here for the main
	// combat update loop to determine damage, reduce item qualities and to provide a message to
	// both players about the strike
	//////////////////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Handle Special Combinations first
	//////////////////////////////////////////////////////////////////////////////////////////////
	// STRIKE vs STRIKE :: Special handling for both players selecting STRIKE
	//////////////////////////////////////////////////////////////////////////////////////////////
	if (pFirst->m_Variables->Var("combat_move") == "strike" && pSecond->m_Variables->Var("combat_move") == "strike")
	{
		SimStrike(pFirst, pSecond);
		return;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////
	// STRIKE vs DODGE or DODGE vs STRIKE :: Dodge can nullify a strike attempt
	//////////////////////////////////////////////////////////////////////////////////////////////
	if ( (pFirst->m_Variables->Var("combat_move") == "strike" && pSecond->m_Variables->Var("combat_move") == "dodge") ||
			(pFirst->m_Variables->Var("combat_move") == "dodge" && pSecond->m_Variables->Var("combat_move") == "strike"))
	{
		// Perform the dodge action to see if the strike has been dodged
		if (pFirst->m_Variables->Var("combat_move") == "dodge")
		{
			if (Dodge(pFirst, pSecond))
			{
				if (pFirst->HasFlag("power"))
				{
					pFirst->m_pWorld->SendCombatMsg(pFirst, pSecond, "dodge", "power");
					pFirst->RemFlag("power");
				}
				else if (pFirst->HasFlag("super"))
				{
					pFirst->m_pWorld->SendCombatMsg(pFirst, pSecond, "dodge", "super");
					pFirst->RemFlag("super");
				}
				else
					pFirst->m_pWorld->SendCombatMsg(pFirst, pSecond, "dodge");
			}
		}

		if (pSecond->m_Variables->Var("combat_move") == "dodge")
		{
			if (Dodge(pSecond, pFirst))
			{
				if (pSecond->HasFlag("power"))
				{
					pSecond->m_pWorld->SendCombatMsg(pSecond, pFirst, "dodge", "power");
					pSecond->RemFlag("power");
				}
				else if (pSecond->HasFlag("super"))
				{
					pSecond->m_pWorld->SendCombatMsg(pSecond, pFirst, "dodge", "super");
					pSecond->RemFlag("super");
				}
				else
					pSecond->m_pWorld->SendCombatMsg(pSecond, pFirst, "dodge");
			}
		}

		// End of round
		Reset(m_pAttacker);
		Reset(m_pDefender);
		return;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////
	// DODGE vs BLOCK or BLOCK vs DODGE :: The player with the higher agility is told this
	//////////////////////////////////////////////////////////////////////////////////////////////
	if ( (pFirst->m_Variables->Var("combat_move") == "block" && pSecond->m_Variables->Var("combat_move") == "dodge") ||
			(pFirst->m_Variables->Var("combat_move") == "dodge" && pSecond->m_Variables->Var("combat_move") == "block"))
	{
		// In this instance, if the dodger has higher agility than his opponent he is told this
		if (pFirst->m_Variables->Var("combat_move") == "dodge" && (pFirst->Attribute("attr_2") > pSecond->Attribute("attr_2")))
		{
			pFirst->Send("&GYou out manoeuvre your opponent.\n\r");
			pSecond->Send("&RYour opponent out manoeuvres you!\n\r");
		}
		else if (pSecond->m_Variables->Var("combat_move") == "dodge" && (pSecond->Attribute("attr_2") > pFirst->Attribute("attr_2")))
		{
			pSecond->Send("&GYou out manoeuvre your opponent.\n\r");
			pFirst->Send("&RYour opponent out manoeuvres you!\n\r");
		}

		return;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////
	// DODGE vs DODGE :: The player with the higher agility is told
	//////////////////////////////////////////////////////////////////////////////////////////////
	if (pFirst->m_Variables->Var("combat_move") == "dodge" && pSecond->m_Variables->Var("combat_move") == "dodge")
	{
		// In this instance, if the dodger has higher agility than his opponent he is told this
		if (pFirst->Attribute("attr_2") > pSecond->Attribute("attr_2"))
		{
			pFirst->Send("&GYou out manoeuvre your opponent.\n\r");
			pSecond->Send("&RYour opponent out manoeuvres you!\n\r");
		}
		else if (pSecond->Attribute("attr_2") > pFirst->Attribute("attr_2"))
		{
			pSecond->Send("&GYou out manoeuvre your opponent.\n\r");
			pFirst->Send("&RYour opponent out manoeuvres you!\n\r");
		}

		return;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////
	// FEINT vs FEINT :: The player with the higher cunning is told
	//////////////////////////////////////////////////////////////////////////////////////////////
	if (pFirst->m_Variables->Var("combat_move") == "feint" && pSecond->m_Variables->Var("combat_move") == "feint")
	{
		// In this instance, if the dodger has higher agility than his opponent he is told this
		if (pFirst->Attribute("attr_4") > pSecond->Attribute("attr_4"))
		{
			pFirst->Send("&GYou out smart your opponent.\n\r");
			pSecond->Send("&RYour opponent out smarts you!n\r");
		}
		else if (pSecond->Attribute("attr_4") > pFirst->Attribute("attr_4"))
		{
			pFirst->Send("&RYour opponent out smarts you!\n\r");
			pSecond->Send("&GYou out smart your opponent.\n\r");
		}

		return;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////
	// A CombatRound is used when the actions are not special cases and allows both players to
	// complete their actions
	//////////////////////////////////////////////////////////////////////////////////////////////
	CombatRound(pFirst, pSecond);

	// Check that pFirst's ACTION didn't kill either player
	if (pFirst->m_Variables->VarI("curr_hp") <= 0 || pSecond->m_Variables->VarI("curr_hp") <= 0)
	{
		End();
		return;
	}

	CombatRound(pSecond, pFirst);

	// Check that pSecond's ACTION didn't kill either player
	if (pFirst->m_Variables->VarI("curr_hp") <= 0 || pSecond->m_Variables->VarI("curr_hp") <= 0)
	{
		End();
		return;
	}

	// Increment the number of rounds
	m_nRounds++;

	// Combat automatically ends after 10 rounds, initial value of nRounds is 0
	if (m_nRounds >= 9)
	{
		Send("The Fight has ended!\n\r");
		End();
		return;
	}

	Reset(m_pAttacker);
	Reset(m_pDefender);
	return;
}

