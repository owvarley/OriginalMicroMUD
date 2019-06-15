/*
 * Main.h
 *
 *  Created on: Mar 15, 2012
 *      Author: Gnarls
 */

#ifndef GAMESOCKET_H_
#define GAMESOCKET_H_

#include "Socket.h"
#include <string>
#include <map>

using namespace std;

// Forward declaration
class GameWorld;
class GameServer;
class Account;
class Player;
class Protocol;
class MSDP;

#define SOCKET_ANSI		 		0
#define SOCKET_ANSI_WAIT 		1
#define SOCKET_CONNECTED 		2
#define SOCKET_GREETING  		3
#define SOCKET_PASSWORD  		4
#define SOCKET_CONFIRM_NAME   	5
#define SOCKET_ENTER_PASSWORD	6
#define SOCKET_EMAIL			7
#define SOCKET_GAMEWORLD		8
#define SOCKET_SPLASH			9
#define SOCKET_PLAY_MENU		10
#define SOCKET_CHARGEN			11
#define SOCKET_CHARGEN_S_NAME	12
#define SOCKET_CHARGEN_RANDOM	13
#define SOCKET_CHARGEN_NAME		14
#define SOCKET_PLAYING   		15

class Player;

class GameSocket : private Socket
{
	public:
		GameSocket();
		GameSocket( int nPort );
		virtual ~GameSocket();

		bool					Accept ( GameSocket * pSock );
		bool					Select ( GameSocket * pSock );
		void					SetNonBlocking();
		std::string				Address();
		sockaddr_in				GetAddress() { return m_addr; }
		int						GetSocket() { return m_sock; }
		void					Send(const string&, ...);
		//bool					Receive(string&);
		bool					Receive(char * s);
		void					Negotiate();
		void					Handshake (char cProtocol, char cCmd);
		void					SubNegotiation (char cProtocol, char str[MAXRECV]);
		void					HandleMSDP (string sStr);
		void					ReportMSDP (string sVal);
		void					HandleMSDPPair (string sVar, string sVal);
		void					MXPSendTag (string sTag);
		string					MXPGetTag (string sTag, string sText);
		void					SendNegotiation(string sStr, ...);
		string					ReceiveNegotiation(char str[MAXRECV]);
		void					Determine256();

		// Commands
		void					Close();
		void					Shutdown(int type);

	// Variables
		int						m_nState;		// State of socket
		int						m_nId;			// Socket ID
		bool					m_bColour;		// Does Socket permit colour?
		bool					m_bDelete;		// Flagged for deletion so ignore
		int						m_nClient;		// Client type
		time_t					m_tLastInput;	// Last time they input anything
		string					m_sRandom;		// Randomly generated name
		Account	*				m_pAccount;		// Point to the Account for the Socket
		Player *				m_pPlayer;		// Player
		GameWorld *				m_pWorld;		// Game world
		GameServer *			m_pServer;		// Game Server
		map<int, bool>			m_Protocols;	// Protocols supported
		map<int, int>			m_Negotiated; 	// Protocols already negotiated
		map<string, string>		m_Variables;	// Socket variables
		map<string, MSDP*>		m_MSDP;			// MSDP Variables

};

#endif /* GAMESOCKET_H_ */
