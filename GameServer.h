/*
 * Main.h
 *
 *  Created on: Mar 15, 2012
 *      Author: Gnarls
 */

#include "Main.h"
#include "GameWorld.h"
#include "Socket.h"
#include "GameSocket.h"
#include "SqliteDbase.h"

#ifndef GAMESERVER_H_
#define GAMESERVER_H_

class GameServer : private Socket
{
	public:
		GameServer();
		GameServer( int nPort );
		virtual ~GameServer();

		void					Initialise(int nPort);	// Boot up server
		void					GameLoop();				// Main Game loop
		bool					HasSocket(GameSocket& pSocket);
		void					AcceptNew ( );
		void					Interpret(GameSocket * pSocket, string sInput);
		bool					Nanny(GameSocket * pSocket, string sInput);
		bool					LoadGameWorlds();
		void					DisplayWorlds(GameSocket * pSocket);
		int						NumPlayers();


		// Server commands
		void					Send(string sMsg);
		void 					Console(const std::string &sMsg, ...);
		void					Shutdown();
		void					Chat(GameSocket * pSocket, string sInput);
		void					Who(GameSocket * pSocket);
		int						NumConnected();
		static time_t			Time();
		static string			StrTime();

		// Protocol support
		void					DetermineProtocols(GameSocket * pSocket);
		void					SendMSSP(GameSocket * pSocket);
		bool					IsProtocol (string sStr);
		bool					IsProtocol (char sStr);




	// Variables
		int						m_nPlayerId;	// Next Player ID to assign
		GameSocket	*			m_pServer;		// Game Socket
		int						m_nPort;		// Port server will run on
		int						m_nSocketId;	// Next Socket ID to assign
		bool					m_bShutdown;	// Are we shutting down?
		vector<GameWorld*>		m_Worlds;		// List of GameWorlds
		vector<GameSocket*>		m_Sockets;		// List of connected sockets
		SqliteDbase *			m_pDbase;		// Database
		list<char>				m_Protocols;	// Protocols the server knows and will support

};

#endif /* MAIN_H_ */
