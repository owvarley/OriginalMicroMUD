/*
 * GameSocket.cpp
 *
 *  Created on: Mar 17, 2012
 *      Author: Gnarls
 */

#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>
#include <arpa/telnet.h>
#include <arpa/inet.h>
#include "GameSocket.h"
#include "GameWorld.h"
#include "SocketException.h"
#include "Main.h"
#include "Protocol.h"
#include "Account.h"
#include "GameServer.h"
#include "MSDP.h"

fd_set        in_set;
fd_set        out_set;
fd_set        exc_set;

GameSocket::GameSocket()
{
	m_nState = SOCKET_ANSI;
	m_bColour = false;
	m_bDelete = false;
	m_nClient = CLIENT_TELNET;
	m_pWorld = NULL;
	m_pAccount = NULL;
	m_tLastInput = GameServer::Time();
	set_non_blocking(true);
}


GameSocket::~GameSocket()
{
	delete m_pAccount;
	m_pAccount = NULL;
	m_pWorld = NULL;
}

GameSocket::GameSocket ( int port )
{
	if ( !Socket::create() )
		throw SocketException ( "Could not create server socket." );

	if ( !Socket::bind ( port ) )
		throw SocketException ( "Could not bind to port." );

	if ( !Socket::listen() )
		throw SocketException ( "Could not listen to socket." );

	std::cout << "[GameServer] Listening on port " << port << std::endl;
}


void GameSocket::Send( const std::string& s, ... )
{
    char buf[MAX_STRING_LENGTH*2];
    va_list args;

    va_start(args, s);
    vsprintf(buf, s.c_str(), args);
    va_end(args);

    string sEdit(buf);

    // Strip colour from non colour supporting clients and add the ANSI codes for those
    // that do support it
 	if (!m_bColour)
 	{
 		size_t found;
 		found = sEdit.find("&");

 		while (found != string::npos)
 		{
 			sEdit.replace(found, 2, "");
 			found = sEdit.find("&");
 		}
 	}
 	else
 	{
 		sEdit.append("&w");
 		// \033[1;31m
 		// ANSI escape character \033
 		// [1 for bold
 		// ;30+xm for colour x(0 black, 1 Red, 2 Green, 3 Yellow, 4 Blue, 5 Purple, 6 Cyan, 7 White
 		//                     z/Z      r/R    g/G      y/Y       b/B     p/p        c/C     w/W
 		size_t found;
		found = sEdit.find("&");

		while (found != string::npos)
		{
			if (sEdit.at(found+1) == 'z')
				sEdit.replace(found, 2, "\033[0;30m");
			else if (sEdit.at(found+1) == 'Z')
				sEdit.replace(found, 2, "\033[1;30m");
			else if (sEdit.at(found+1) == 'r')
				sEdit.replace(found, 2, "\033[0;31m");
			else if (sEdit.at(found+1) == 'R')
				sEdit.replace(found, 2, "\033[1;31m");
			else if (sEdit.at(found+1) == 'g')
				sEdit.replace(found, 2, "\033[0;32m");
			else if (sEdit.at(found+1) == 'G')
				sEdit.replace(found, 2, "\033[1;32m");
			else if (sEdit.at(found+1) == 'y')
				sEdit.replace(found, 2, "\033[0;33m");
			else if (sEdit.at(found+1) == 'Y')
				sEdit.replace(found, 2, "\033[1;33m");
			else if (sEdit.at(found+1) == 'b')
				sEdit.replace(found, 2, "\033[0;34m");
			else if (sEdit.at(found+1) == 'B')
				sEdit.replace(found, 2, "\033[1;34m");
			else if (sEdit.at(found+1) == 'p')
				sEdit.replace(found, 2, "\033[0;35m");
			else if (sEdit.at(found+1) == 'P')
				sEdit.replace(found, 2, "\033[1;35m");
			else if (sEdit.at(found+1) == 'c')
				sEdit.replace(found, 2, "\033[0;36m");
			else if (sEdit.at(found+1) == 'C')
				sEdit.replace(found, 2, "\033[1;36m");
			else if (sEdit.at(found+1) == 'w')
				sEdit.replace(found, 2, "\033[0m");
			else if (sEdit.at(found+1) == 'W')
				sEdit.replace(found, 2, "\033[1;37m");
			else
				sEdit.replace(found, 2, "");

			found = sEdit.find("&");
		}

 	}

	if ( ! Socket::send ( sEdit ) )
		throw SocketException ( "Could not write to socket." );

	return;
}

bool GameSocket::Receive( char s[MAXRECV] )
{
	if ( ! Socket::recv ( s ) )
		return false;

	//throw SocketException ( "Could not read from socket." );

	return true;
}

bool GameSocket::Select( GameSocket * sock )
{
	struct timeval tv;
	fd_set newconns;

	tv.tv_sec = 2;
	tv.tv_usec = 500000;

	FD_ZERO(&newconns);
	FD_SET(m_sock, &newconns);

	Socket::select ( sock, m_sock, &newconns, NULL, NULL, &tv );

	if (FD_ISSET(m_sock, &newconns))
		return true;
	else
		return false;
}

void GameSocket::SetNonBlocking()
{
	Socket::set_non_blocking(true);
}

bool GameSocket::Accept ( GameSocket * sock )
{
	if ( Socket::accept( sock ) )
		return true;
	else
		return false;

		//throw SocketException ( "Could not accept socket." );
}

void GameSocket::Close()
{
	// Close will return a value of 0 if it closed successfully
	if ( Socket::close() != 0 )
		throw SocketException ( "Could not close socket." );

}

void GameSocket::Shutdown(int type)
{
	if ( ! Socket::shutdown(type) )
		throw SocketException ( "Could not shutdown socket." );
}

std::string GameSocket::Address()
{
	char str[INET6_ADDRSTRLEN];

	inet_ntop(AF_INET, &(this->m_addr.sin_addr), str, INET_ADDRSTRLEN);

	std::stringstream sOutput;

	sOutput << str;

	return sOutput.str();

}

// Telnet negotiation
// Determine the protocols and options that are supported by the client on first connection. We will
// check for TTYPE support first and we will only send TELNET commands to clients that support this to
// handle the GMUD issues
void GameSocket::Negotiate()
{
	SendNegotiation("IAC DO TTYPE");
}

// Attempt to handshake for a given protocol. When the server sends out an IAC Command it will add
// an entry in the m_Negotiated map for the given protocol to record that it has sent a request (SEND)
// when the client responds it will check m_Negotiated and will set the protocol's value to either (ACCEPTED)
// or (REJECTED). This will also handle requests by the client to disable/enable protocols. The GameServer
// has a list of protocols that it will support (m_Protocols) and it will respond as required to interrogation.
void GameSocket::Handshake (char cProtocol, char cCmd)
{
	// Check the protocol is valid
	if (!m_pServer->IsProtocol(cProtocol))
		return;

	// Check we have a valid command
	if (!Protocol::IsHandShake(cCmd))
		return;

	// Display if debugging has been enabled
	if (PROTOCOL_DEBUG)
		m_pServer->Console("[Client] IAC %s %s", Protocol::ConvertTelnetToString(cCmd, "").c_str(), Protocol::ConvertTelnetToString(cProtocol, "").c_str());

	// Handle the request to enable/disable a given protocol
	bool bResponse = Protocol::ConvertDoWill(cCmd);

	// First handle responses to our requests
	// SEND indicates that we have sent a request to the client to use this protocol
	if (m_Negotiated[cProtocol] == SEND)
	{
		if (bResponse)
			m_Negotiated[cProtocol] = ACCEPTED;		// Client responded positively so enable this protocol
		else
			m_Negotiated[cProtocol] = REJECTED;		// Client responded negatively so disable it

		// We open with TTYPE as if they don't support this we wont bother with further protocols
		if (cProtocol == TELOPT_TTYPE && bResponse)
		{
			Send("%c%c%c%c%c%c\0", (char)IAC, (char)SB, TELOPT_TTYPE, (char)SEND, (char)IAC, (char)SE);
			if (PROTOCOL_DEBUG)
				m_pServer->Console("[Server] IAC SB TTYPE SEND IAC SE");

			return;
		}

		// We have to handle both IAC WILL MXP and IAC DO MXP as some clients support one and not the other
		if (cProtocol == TELOPT_MXP)
		{
			if  (!bResponse)
			{
				if (cCmd == (char)WONT)
					Send("%c%c%c", (char)IAC, (char)DO, TELOPT_MXP);
				else if (cCmd == (char)DONT)
					Send("%c%c%c", (char)IAC, (char)WILL, TELOPT_MXP);
			}
			else
			{
				// Enable MXP
				Send("%c%c%c%c%c", (char)IAC, (char)SB, TELOPT_MXP, (char)IAC, (char)SE);
				if (PROTOCOL_DEBUG)
					m_pServer->Console("[Server] IAC SB MXP IAC SE");
				// Establish MXP Channel and set default as locked
				Send("\033[7z");
				// Get Version information
				MXPSendTag ("<VERSION>");
			}

			return;
		}

		// Handle a DO MSSP response
		if (cProtocol == TELOPT_MSSP && bResponse)
		{
			m_pServer->SendMSSP(this);
			return;
		}

	}

	// No negotiation received so the client is attempting a Handshake, respond appropriately
	if (m_Negotiated[cProtocol] == NOTHING)
	{
		// Our response to this handshake is determined by what protocols we have selected the server
		// will accept
		if (m_pServer->IsProtocol(cProtocol))
		{
			if (cCmd == (char)WILL)
				SendNegotiation("IAC DO %s", Protocol::ConvertTelnetToString(cProtocol, "").c_str());	// Correct response to WILL is DO
			else
				SendNegotiation("IAC WILL %s", Protocol::ConvertTelnetToString(cProtocol, "").c_str());	// Correct response to DO is WILL

			m_Negotiated[cProtocol] = ACCEPTED;
		}
		else
		{
			if (cCmd == (char)WILL)
				SendNegotiation("IAC WONT %s", Protocol::ConvertTelnetToString(cProtocol, "").c_str());	// Correct response to WILL is WONT
			else
				SendNegotiation("IAC DONT %s", Protocol::ConvertTelnetToString(cProtocol, "").c_str());	// Correct response to DO is DONT

			m_Negotiated[cProtocol] = REJECTED;
		}

	}

	return;
}

// Handle a request for sub negotiation for a given protocol
// The contents of sStr will vary between protocols and this function should deal with
// it all
void GameSocket::SubNegotiation (char cProtocol, char sStr[MAXRECV])
{
	// [1] MSDP Handling
	// If we are using MSDP then we need to substitute for the MSDP defines and handle the string
	if (cProtocol == TELOPT_MSDP)
	{
		HandleMSDP (sStr);
	}

	// [2] Handle Negotatiate About Windows Size (NAWS)
	if (cProtocol == TELOPT_NAWS)
	{
		m_Variables["col_hi"]  = (int)sStr[0];
		m_Variables["col_lo"]  = (int)sStr[1];
		m_Variables["line_hi"] = (int)sStr[2];
		m_Variables["line_lo"] = (int)sStr[3];

		if (PROTOCOL_DEBUG)
			m_pServer->Console("[Client] IAC SB NAWS %d %d %d %d IAC SE", (int)sStr[0], (int)sStr[1], (int)sStr[2], (int)sStr[3] );
	}

	// [3] Handle TTYPE
	if (cProtocol == TELOPT_TTYPE)
	{
		string sClient;

		for (int Index = 0; Index < MAXRECV; Index++)
		{
			if (sStr[Index] == '\0' && sStr[Index+1] != '\0')
				continue;

			if (sStr[Index] == '\0' && sStr[Index+1] == '\0')
				break;
			else
				sClient.push_back(sStr[Index]);
		}

		// If we got a client save it and pass it back to the client
		if (!sClient.empty())
		{
			// Don't overwrite the client if MXP has already filled it. This shouldn't happen
			// as we always send TTYPE first, however, just to be sure!
			if (m_Variables["client"].empty())
				m_Variables["client"] = sClient;

			if (PROTOCOL_DEBUG)
				m_pServer->Console("[Client] IAC SB TTYPE %s IAC SE", sClient.c_str());

			// TinTin++ WinTin++ BlowTorch 256 colour detection
			if (sClient.find("-256color") != string::npos)
				m_Variables["256colour"] = "yes";

			// We know that EMACS and DecafMUD both have support for 256 colour
			if (sClient == "EMACS-RINZAI" || sClient == "DecafMUD")
				m_Variables["256colour"] = "yes";
		}

		// TTYPE completed so send further negotiation
		Send("Do you wish to enable Accessibility mode? (Y/N)\n\r");
		SendNegotiation("IAC WILL MSDP");
		SendNegotiation("IAC WILL MSP");
		SendNegotiation("IAC WILL MXP");
		SendNegotiation("IAC WILL MSSP");


	}

	// [4] Handle MXP
	if (cProtocol == TELOPT_MXP)
	{

	}

	// [5] Handle MSSP
	if (cProtocol == TELOPT_MSSP)
	{
		if (m_pServer)
			m_pServer->SendMSSP(this);
	}
}

// Handle MXP
void GameSocket::MXPSendTag (string sTag)
{
	if (m_Negotiated[TELOPT_MXP] == ACCEPTED)
	{
		Send("\033[1z%s\033[7z", sTag.c_str());
	}
	return;
}

// Receive and handle an MXP tag
// Taken from the Kavir's Protocol Snippet and modified for MicroMud usage
// Kudos to Kavir for his excellent and easy to follow code
string GameSocket::MXPGetTag (string sTag, string sText)
{
	stringstream ssStr(sText);
	size_t found;
	string sReturn = "";


	// Try to locate the tag within the string
	if ((found = sText.find(sTag)) != string::npos)
	{
		// Go from the end of the tag
		for (size_t Index = found + sTag.size(); Index < sText.size(); Index++)
		{
			char Letter = sText[Index];

			if ( Letter == '\"')
				continue;

			if ( Letter == '.' || isdigit(Letter) || isalpha(Letter) )
			{
				sReturn.push_back(Letter);
			}
			else
			{
				sReturn.push_back('\0');
				return sReturn;
			}
		}
	}
	else
		return "";


	// No tag found
	return "";
}

// Handle MSDP input
void GameSocket::HandleMSDP (string sStr)
{
	map<string, string> m_MSDP;
	stringstream ssStr;
	string sWord;
	string sVar = "";
	string sVal = "";

	// Iterate through the string and convert it all to a string replacing any MSDP specific variables (VAR/VAL/ETC)
	for (size_t Index = 0; Index < sStr.size(); Index++)
	{
		if (Protocol::IsMSDP(sStr[Index]))
			ssStr << Protocol::ConvertMSDP(sStr[Index]);
		else
			ssStr << sStr[Index];
	}

	bool bReport = false;

	// Format for MSDP is:
	// IAC SB MSDP MSDP_VAR "VARIABLE" MSDP_VAL "VALUE"
	// Has to handle MSDP_ARRAY_OPEN and MSDP_TABLE_CLOSE as well
	while (ssStr >> sWord)
	{
		// If we receive a request from the client to report then we need to start handling input as
		// values to report
		if (sWord == "REPORT")
		{
			bReport = true;
			continue;
		}

		if (bReport)
		{
			if (sWord == "MSDP_VAR" || sWord == "MSDP_VAL")
				continue;

			// If we have encountered a REPORT Variable then all input is considered to be a value
			// that we want to start reporting
			if (bReport)
				ReportMSDP(sWord);
		}

		m_pServer->Console ("%s", sWord.c_str());
	}
}

// Set a MSDP variable as reportable
void GameSocket::ReportMSDP (string sVar)
{
	if (m_MSDP[sVar])
	{
		m_MSDP[sVar]->m_bReport = true;
	}
	else
	{
		MSDP * pMSDP = new MSDP;
		pMSDP->m_bReport = true;
		pMSDP->m_sVariable = sVar;
		m_MSDP[sVar] = pMSDP;
	}

	// Send("Reporting %s\n\r", sVar.c_str());
	return;
}

// Handle a MSDP pair
void GameSocket::HandleMSDPPair (string sVar, string sVal)
{
	//m_pServer->Console("[MSDP] Var: %s Val: %s", sVar.c_str(), sVal.c_str());
}

// Send a command to the client, this will convert a human readable string into the
// required char sequence for protocol negotiation. For example a string of IAC WILL TTYPE
// will be converted to \\377\\373\\030 and an entry will be added in the m_Negotiated map
// that we attempted to negotiate
void GameSocket::SendNegotiation (string sStr, ...)
{
    char buf[MAX_STRING_LENGTH*2];
    va_list args;

    va_start(args, sStr);
    vsprintf(buf, sStr.c_str(), args);
    va_end(args);

    string sEdit(buf);

	// Iterate through each word in the String
	stringstream ssStr(sEdit);
	stringstream ssConsole;
	string sOutput;
	string sWord;
	char cProtocol;
	char cCommand;

	bool bConvert = true;

	// Iterate through each word and convert it to its corresponding char representation
	// Manages strings in the format:
	// [1] IAC WILL/WONT/DO/DONT PROTOCOL
	// [2] IAC SB PROTOCOL ... IAC SE
	while (ssStr >> sWord)
	{
		// We won't try to convert any of the ouput after the subnegotiation indicator
		if (sWord == "SB")
			bConvert = false;

		// We have now finished subnegotiation so we will convert all output
		if (sWord == "SE")
			bConvert = true;

		if (bConvert)
		{
			char cCh = Protocol::ConvertTelnetToChar(sWord);
			Send("%c", cCh);

			// Check if the word is a Protocol or Command (Will/Wont/Do/Dont)
			// this will only be checked for converted words
			if (m_pServer->IsProtocol(cCh))
				cProtocol = cCh;
			if (Protocol::IsHandShake(cCh))
				cCommand = cCh;
		}
		else
		{
			for (size_t i = 0; i < sWord.length(); i++)
			{
				if (isdigit(sWord[i]))
					Send("%d", atoi(sWord.c_str()));
				else
					Send("%c", (char) sWord[i]);
			}

		}

		ssConsole << sWord << " ";
	}

	// End command
	Send("\0");

	// Update the negotiation state for this user
	if (cProtocol && cCommand)
	{
		// There is no entry for this protocol and we are sending a negotiation so we set the
		// state to SEND, this indicates that this is the Server attempting to negotiate with
		// the client.
		if (m_Negotiated[cProtocol] == NOTHING)
			m_Negotiated[cProtocol] = SEND;
	}

	if (PROTOCOL_DEBUG && ssConsole.str().size() > 0)
		m_pServer->Console("[Server] %s", ssConsole.str().c_str());

	return;
}

// Check an input string for any Telnet negotiations and remove them from the string
// whilst handling them
string GameSocket::ReceiveNegotiation (char sStr[MAXRECV])
{
	string sReturn = "";		// This string will contain any input that is not MXP/OOB

	bool bIAC = false;
	bool bSub = false;

	// Take the stream of input from the Client and convert it all into human readable strings, this
	// will work assuming that the only two forms of input are:
	// [1] IAC WILL/WONT/DO/DONT PROTOCOL
	// [2] IAC SB PROTOCOL .... IAC SE
	// For the [2] any of the input between PROTOCOL and IAC will NOT be converted using the Protocol
	// string conversion and will just be copied across normally. This will prevent problems when the client
	// sends input such as the character 'E' which is the same as the MSDP define
	for (size_t Index = 0; Index < MAX_INPUT_LENGTH; Index++)
	{
		// Wait for the first IAC before we do anything
		if (sStr[Index] == (char)IAC && !bIAC)
		{
			bIAC = true;
			continue;
		}

		// Skip CR and LR
		//if (sStr[Index] == '\r' || sStr[Index] == '\n')
		//	continue;

		// Check for MXP input
		// Looking for \[nz
		if (sStr[Index] == (char)27 && sStr[Index+1] == '[' && isdigit(sStr[Index+2]) && sStr[Index+3] == 'z')
		{
			Index += 4;		// Skip the initial input (\[nz) bit
			char MXPBuffer[MAXRECV];
			int nCount = 0;

			while (sStr[Index] != '>' && nCount < MAXRECV)
			{
				MXPBuffer[nCount++] = sStr[Index++];
			}

			// Increment the Index counter to skip the >
			Index++;

			// Remove any trailing \n or \rs
			if (sStr[Index+1] == '\n' || sStr[Index+1] == '\r')
				Index++;

			// Do it twice!
			if (sStr[Index+1] == '\n' || sStr[Index+1] == '\r')
				Index++;

			MXPBuffer[nCount++] = '>';
			MXPBuffer[nCount] = '\0';

			string sMXP = MXPBuffer;
			string sMXPReturn;

			if ( !(sMXPReturn = MXPGetTag("VERSION MXP=", sMXP)).empty() )
				m_Variables["mxp_version"] = sMXPReturn.c_str();

			if ( !(sMXPReturn = MXPGetTag("CLIENT=", sMXP)).empty() )
				m_Variables["client"] = sMXPReturn.c_str();

			if ( !(sMXPReturn = MXPGetTag("VERSION=", sMXP)).empty() )
				m_Variables["client_version"] = sMXPReturn.c_str();

			if ( !(sMXPReturn = MXPGetTag("REGISTERED=", sMXP)).empty() )
				m_Variables["client_registered"] = sMXPReturn.c_str();

			continue;
		}

		if (!bIAC)
		{
			sReturn.push_back(sStr[Index]);
			continue;
		}

		// Ok, we got IAC in the last loop so the next input is either going to be a command (WILL/WONT/DO/DONT)
		// or SB
		if (bIAC)
		{
			// [1] - Check if we received a Command, if so we need to perform a handshake
			if (Protocol::IsHandShake(sStr[Index]))
			{
				// We received a command so we need to handle this handshake attempt
				// we know the next piece of input should be the protocol so lets look for this
				if (m_pServer->IsProtocol(sStr[Index+1]))
				{
					// Handshake
					Handshake (sStr[Index+1], sStr[Index]);
					bIAC = false;
					Index++;	// Skip the protocol

					// Is this a single line of communication terminated with a \0? If so remove it
					if (sStr[Index+1] == '\0')
						Index++;

					continue;
				}
			}
			else
			{
				// No command received, is it SB or SE?
				if (sStr[Index] == (char)SB)
				{
					bSub = true;
					continue;
				}

				// Did we just receive SE? if so its the end of our subnegotiation
				if (sStr[Index] == (char)SE)
				{
					bSub = false;
					continue;
				}

				// Are we in a sub negotiation?
				if (bSub)
				{
					// For a subnegotiation we want all text between the protocol and the IAC at the
					// end. We will concatenate these together and pass the protocol and text to the
					// subnegotation function
					if (m_pServer->IsProtocol(sStr[Index]))
					{
						size_t nCount = Index+1;		// Start the counter on the next char after the protocol
						char cProtocol = sStr[Index];	// Add the protocol (entry immediately following SB)
						char sSub[MAX_STRING_LENGTH];	// Contains the text to subnegotiate over
						memset (sSub, 0, MAX_STRING_LENGTH + 1);
						int nIndex = 0;

						// Obtain the rest of the text
						while (sStr[nCount] != (char)IAC && nCount < MAX_INPUT_LENGTH)
						{
							sSub[nIndex] =  sStr[nCount];
							nCount++;
							nIndex++;
						}

						SubNegotiation (cProtocol, sSub);
						bSub = false;
						Index = nCount - 1;	// Set the loop counter manually so that we don't look at the
											// sub negotiation text twice
						continue;
					}
				}
			}
		}
	}

	// sReturn now contains the string we want to reply, to make this compatible in later code
	// we have to create a new string and construct it using the data gathered in sReturn
	// without doing this simple comparisons are not possible
	string sReply(sReturn.data());

	return sReply;
}

// Check if the user's client can handle 256 colours and if so
// set the relevant variable
void GameSocket::Determine256()
{
	// Already handled
	if (m_Variables["256colour"] == "yes")
		return;

	// MushClient version 4.02 and above supports
	if (StrEquals(m_Variables["client"], "MUSHCLIENT") && strcmp(m_Variables["client_version"].c_str(), "4.02") >= 0)
		m_Variables["256colour"] = "yes";

	// CMUD version 3.04 and above supports
	if (StrEquals(m_Variables["client"], "CMUD") && strcmp(m_Variables["client_version"].c_str(), "4.02") >= 0)
		m_Variables["256colour"] = "yes";

	// Atlantis suppots
	if (StrEquals(m_Variables["client"], "ATLANTIS"))
		m_Variables["256colour"] = "yes";

	return;
}

