/*
 * Variable.cpp
 *
 *  Created on: Aug 26, 2012
 *      Author: owen
 */

#include "Variable.h"
#include <sstream>

Variable::Variable()
{
	m_sValue = "";
	m_nValue = 0;
	m_bInt = false;		// Default is a string
}

Variable::~Variable()
{
	m_sValue = "";
	m_nValue = 0;
}

string Variable::Get()
{
	if (this == NULL)
		return "";

	if (m_nValue != 0)
	{
		stringstream sVal;
		sVal << m_nValue;
		return sVal.str();
	}
	else
		return m_sValue;
}

int Variable::GetI()
{
	// Has to handle having a null object to allow quick calling
	if (this == NULL)
		return 0;

	return m_nValue;
}

void Variable::Set(int nValue)
{
	m_nValue = nValue;
	m_bInt = true;
}

void Variable::Set(string sValue)
{
	m_sValue = sValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// VARIABLE MANAGER
/////////////////////////////////////////////////////////////////////////////////////////////////////////

VariableManager::VariableManager()
{

}

VariableManager::~VariableManager()
{
	for (map<string, Variable*>::iterator it = m_Variables.begin(); it != m_Variables.end(); it++)
		delete (*it).second;
}

// SETTERS

void VariableManager::Set (string sVariable, string sValue)
{
	if (HasVar(sVariable))
	{
		m_Variables[sVariable]->m_sValue = sValue;
		m_Variables[sVariable]->m_bInt = false;
	}
	else
		AddVar(sVariable, sValue);

	return;
}

void VariableManager::Set (string sVariable, int nVal)
{
	if (HasVar(sVariable))
	{
		m_Variables[sVariable]->m_nValue = nVal;
		m_Variables[sVariable]->m_bInt = true;
	}
	else
		AddVar(sVariable, nVal);

	return;
}

// GETTERS

string VariableManager::Var (string sVar)
{
	if (HasVar(sVar))
		return m_Variables[sVar]->m_sValue;

	return "";
}

int VariableManager::VarI (string sVar)
{
	if (HasVar(sVar))
		return m_Variables[sVar]->m_nValue;

	return 0;
}

// ANCHILLARY FUNCTIONS

bool VariableManager::HasVar(string sVar)
{
	if (m_Variables.find(sVar) != m_Variables.end())
		return true;

	return false;
}

// MANIPULATORS

void VariableManager::RemVar(string sVar)
{
	if (HasVar(sVar))
		m_Variables.erase(sVar);
}

void VariableManager::AddToVar(string sVar, int nAmount)
{
	if (!HasVar(sVar))
		return;

	m_Variables[sVar]->m_nValue += nAmount;
	return;
}

void VariableManager::AddVar(string sVar, int nValue)
{
	if (HasVar(sVar))
		return;

	Variable * pVariable = new Variable;
	pVariable->m_nValue = nValue;
	pVariable->m_bInt = true;
	m_Variables[sVar] = pVariable;
	return;
}

void VariableManager::AddVar(string sVar, string sValue)
{
	if (HasVar(sVar))
		return;

	Variable * pVariable = new Variable;
	pVariable->m_sValue = sValue;
	m_Variables[sVar] = pVariable;
	return;
}

