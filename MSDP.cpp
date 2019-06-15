/*
 * MSDP.cpp
 *
 *  Created on: Sep 2, 2012
 *      Author: owen
 */

#include "MSDP.h"

MSDP::MSDP()
{
	m_bReport = false;
	m_bChanged = false;
}

MSDP::~MSDP()
{
	m_sVariable = "";
	m_sValue = "";
}

