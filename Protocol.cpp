/*
 * Protocol.cpp
 *
 *  Created on: Aug 28, 2012
 *      Author: owen
 */

#include "Protocol.h"
#include <arpa/telnet.h>
#include <stdarg.h>
#include <cctype>
#include "GameSocket.h"
#include "GameWorld.h"
#include "Main.h"
#include <map>

Protocol::Protocol()
{
	// TODO Auto-generated constructor stub

}

Protocol::~Protocol()
{
	// TODO Auto-generated destructor stub
}


// This will replace any MSDP variables in the string with the human readable code
string Protocol::ReplaceMSDP (string sStr)
{
	return "";
}

// This will convert a string representation of a protocol into its client readable format. For example
// it will convert IAC into 255, TTYPE into 18, MSDP_VAR into 1, etc
char Protocol::ConvertTelnetToChar (string sEntry)
{
	map<string, char> m_Conversions;

	// Telnet Options
	m_Conversions["CHARSET"] = TELOPT_CHARSET;
	m_Conversions["MSDP"] = TELOPT_MSDP;
	m_Conversions["MSSP"] = TELOPT_MSSP;
	m_Conversions["MCCP"] = TELOPT_MCCP;
	m_Conversions["MSP"] = TELOPT_MSP;
	m_Conversions["MXP"] = TELOPT_MXP;
	m_Conversions["ATCP"] = TELOPT_ATCP;
	m_Conversions["ECHO"] = TELOPT_ECHO;
	m_Conversions["TTYPE"] = TELOPT_TTYPE;
	m_Conversions["NAWS"] = TELOPT_NAWS;

	// MSDP
	m_Conversions["MSDP_VAR"] = (char)MSDP_VAR;
	m_Conversions["MSDP_VAL"] = (char)MSDP_VAL;
	m_Conversions["MSDP_TABLE_OPEN"] = (char)MSDP_TABLE_OPEN;
	m_Conversions["MSDP_TABLE_CLOSE"] = (char)MSDP_TABLE_CLOSE;
	m_Conversions["MSDP_ARRAY_OPEN"] = (char)MSDP_ARRAY_OPEN;
	m_Conversions["MSDP_ARRAY_CLOSE"] = (char)MSDP_ARRAY_CLOSE;

	// Commands
	m_Conversions["SEND"] = (char)SEND;
	m_Conversions["ACCEPTED"] = (char)ACCEPTED;
	m_Conversions["REJECTED"] = (char)REJECTED;

	// Telnet
	m_Conversions["IAC"] = (char)IAC;
	m_Conversions["SB"] = (char)SB;
	m_Conversions["SE"] = (char)SE;
	m_Conversions["WILL"] = (char)WILL;
	m_Conversions["WONT"] = (char)WONT;
	m_Conversions["DO"] = (char)DO;
	m_Conversions["DONT"] = (char)DONT;

	return m_Conversions[sEntry];
}

// Takes a character that has been sent to us by the client and converts it into a human readable string
// this has to be clever enough to know the context of its conversion as certain fields have the same value
// for example SEND is defined as 1 which is the same as MSDP_VAR
string Protocol::ConvertTelnetToString (char cEntry, string sScope)
{
	map <char, string> m_ConvTelOpts;

	// Telnet Options conversion
	m_ConvTelOpts[TELOPT_CHARSET] = "CHARSET";
	m_ConvTelOpts[TELOPT_MSDP] = "MSDP";
	m_ConvTelOpts[TELOPT_MSSP] = "MSSP";
	m_ConvTelOpts[TELOPT_MCCP] = "MCCP";
	m_ConvTelOpts[TELOPT_MSP] = "MSP";
	m_ConvTelOpts[TELOPT_MXP] = "MXP";
	m_ConvTelOpts[TELOPT_ATCP] = "ATCP";
	m_ConvTelOpts[TELOPT_ECHO] = "ECHO";
	m_ConvTelOpts[TELOPT_TTYPE] = "TTYPE";
	m_ConvTelOpts[TELOPT_NAWS] = "NAWS";
	if (sScope == "MSDP")
	{
		m_ConvTelOpts[(char)MSDP_VAR] = "MSDP_VAR";
		m_ConvTelOpts[(char)MSDP_VAL] = "MSDP_VAL";
		m_ConvTelOpts[(char)MSDP_TABLE_OPEN] = "MSDP_TABLE_OPEN";
		m_ConvTelOpts[(char)MSDP_TABLE_CLOSE] = "MSDP_TABLE_CLOSE";
		m_ConvTelOpts[(char)MSDP_ARRAY_OPEN] = "MSDP_ARRAY_OPEN";
		m_ConvTelOpts[(char)MSDP_ARRAY_CLOSE] = "MSDP_ARRAY_CLOSE";
	}
	else
	{
		m_ConvTelOpts[(char)SEND] = "SEND";
		m_ConvTelOpts[(char)ACCEPTED] = "ACCEPTED";
		m_ConvTelOpts[(char)REJECTED] = "REJECTED";
	}
	// Other telnet commands to convert
	m_ConvTelOpts[(char)IAC] = "IAC";
	m_ConvTelOpts[(char)SB] = "SB";
	m_ConvTelOpts[(char)SE] = "SE";
	m_ConvTelOpts[(char)WILL] = "WILL";
	m_ConvTelOpts[(char)WONT] = "WONT";
	m_ConvTelOpts[(char)DO] = "DO";
	m_ConvTelOpts[(char)DONT] = "DONT";

	string sReturn;

	// If we can't find a conversion for this entry then just return the entry itself
	if (m_ConvTelOpts.find(cEntry) != m_ConvTelOpts.end())
		sReturn = m_ConvTelOpts[cEntry];
	else
		sReturn.push_back(cEntry);

	return sReturn;
}

string Protocol::ConvertMSDP (char sEntry)
{
	switch (sEntry)
	{
	case MSDP_VAR:
		return " MSDP_VAR ";
	case MSDP_VAL:
		return " MSDP_VAL ";
	case MSDP_ARRAY_OPEN:
		return " MSDP_ARRAY_OPEN ";
	case MSDP_ARRAY_CLOSE:
		return " MSDP_ARRAY_CLOSE ";
	case MSDP_TABLE_OPEN:
		return " MSDP_TABLE_OPEN ";
	case MSDP_TABLE_CLOSE:
		return " MSDP_TABLE_CLOSE ";

	default:
		return "";
	}
}

bool Protocol::IsMSDP (char sEntry)
{
	switch (sEntry)
	{
	case MSDP_VAR:
	case MSDP_VAL:
	case MSDP_ARRAY_OPEN:
	case MSDP_ARRAY_CLOSE:
	case MSDP_TABLE_OPEN:
	case MSDP_TABLE_CLOSE:
		return true;

	default:
		return false;
	}
}

bool Protocol::IsHandShake (char cEntry)
{
	if (cEntry == (char)WILL || cEntry == (char)WONT || cEntry == (char)DO || cEntry == (char)DONT)
		return true;

	return false;
}

bool Protocol::ConvertDoWill (char sWord)
{
	if (sWord == (char)DO || sWord == (char)WILL)
		return true;
	else
		return false;
}
