/*
 * Combat.cpp
 *
 *  Created on: Sep 19, 2012
 *      Author: owen
 */

#include <stdarg.h>
#include "Combat.h"
#include "Actor.h"
#include "Main.h"

Combat::Combat()
{
	// TODO Auto-generated constructor stub
}

Combat::~Combat()
{
	// TODO Auto-generated destructor stub
}

void Combat::Update()
{

}

void Combat::End()
{

}

void Combat::Reset(Actor * pActor)
{
	pActor->m_Variables->RemVar("combat_move");
	pActor->m_Variables->RemVar("combat_move_time");
	pActor->m_Variables->RemVar("combat_stance");
	pActor->RemFlag("initiative");

	if (pActor->IsAffected("cornered"))
		pActor->RemAffect("cornered");
	if (pActor->IsAffected("overextended"))
		pActor->RemAffect("overextended");
}

// Reset - Called after a strike
void Combat::ResetStrike()
{
	m_bBlock = false;
	m_bResist = false;
	m_nDamage = 0;
}

void Combat::Send (string sMes, ...)
{
    char buf[MAX_STRING_LENGTH*2];
    va_list args;

    va_start(args, sMes);
    vsprintf(buf, sMes.c_str(), args);
    va_end(args);

    m_pAttacker->Send(buf);
    m_pDefender->Send(buf);
    return;
}

CombatMessage::CombatMessage(string sMove, string sWeapon, string sState, string sMessage)
{
	m_sMove 	= sMove;
	m_sWeapon 	= sWeapon;
	m_sState	= sState;
	m_sMessage	= sMessage;
}

CombatMessage::~CombatMessage()
{
	// TODO Auto-generated destructor stub
}
