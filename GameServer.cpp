//============================================================================
// Name        : GameServer
// Author      : Owen Varley
// Version     : v 0.1
// Date        : Project Started 15 Mar 12
// Copyright   : Hmm... will need this!
// Description : The GameServer class handles all sockets and descriptors
//============================================================================

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <algorithm>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>
#include <unistd.h>
#include "GameServer.h"
#include "GameSocket.h"
#include "SocketException.h"
#include "SqliteDbase.h"
#include "GameWorlds/Shinobi.h"
#include "Libraries/tinyxml2.h"
#include "Account.h"
#include "Protocol.h"
#include "Player.h"
#include "Areas/Area.h"

using namespace std;
using namespace tinyxml2;

void Error(const char *msg)
{
    perror(msg);
    exit(1);
}

GameServer::GameServer()
{
	m_nSocketId = 0;
	m_bShutdown = false;
	m_pDbase = NULL;
	m_nPlayerId = 1;

	// Supported TELNET protocol
	m_Protocols.push_back(TELOPT_CHARSET);
	m_Protocols.push_back(TELOPT_MSDP);
	m_Protocols.push_back(TELOPT_MSSP);
	m_Protocols.push_back(TELOPT_MCCP);
	m_Protocols.push_back(TELOPT_MSP);
	m_Protocols.push_back(TELOPT_MXP);
	m_Protocols.push_back(TELOPT_ATCP);
	m_Protocols.push_back(TELOPT_ECHO);
	m_Protocols.push_back(TELOPT_TTYPE);
	m_Protocols.push_back(TELOPT_NAWS);
}

GameServer::~GameServer()
{
	for (vector<GameWorld*>::iterator it = m_Worlds.begin(); it != m_Worlds.end(); it ++)
	{
		delete *it;
		it = m_Worlds.begin();
	}

	m_Protocols.clear();
}

// GameServer Interpreter
// Figure out what the connected socket wants to do based on their input
void GameServer::Interpret(GameSocket * pSocket, string sInput)
{
	// Remove any carriage returns prior to comparison
	sInput.erase(std::remove(sInput.begin(), sInput.end(), '\n'), sInput.end());
	sInput.erase(std::remove(sInput.begin(), sInput.end(), '\r'), sInput.end());

	// Handle initial connections
	if (pSocket->m_nState == SOCKET_ANSI_WAIT)
	{
		string sTest = "y";

		if (sInput == "y")
		{
			pSocket->m_bColour = false;
			pSocket->m_Variables["accessibility"] = "yes";		// Enable accessibility mode for this user
			pSocket->Send("Output will be configured for screen readers.\n\r");
			pSocket->m_nState = SOCKET_CONNECTED;
			return;
		}
		else if (sInput == "n")
		{
			pSocket->m_bColour = true;


			pSocket->m_nState = SOCKET_CONNECTED;
			return;
		}
		else
		{
			// This is where our custom clients send their types
			if (sInput == "WEB_CLIENT")
			{
				pSocket->Send("Web Client connected.\n\r");
				pSocket->m_nClient = CLIENT_WEB;
				pSocket->m_nState = SOCKET_CONNECTED;
			}
			else if (sInput == "MOBILE_CLIENT")
			{
				pSocket->Send("Mobile Client connected.\n\r");
				pSocket->m_nClient = CLIENT_MOBILE;
				pSocket->m_nState = SOCKET_CONNECTED;
			}
			else
			{
				pSocket->Send("That is not a valid selection. Please enter Y or N.\n\r");
				return;
			}
		}
	}

	if (pSocket->m_nClient == CLIENT_WEB)
	{
		if (sInput == "who")
		{
			pSocket->Send("%d", NumConnected());
			return;
		}
	}

	// Handle Account creation
	if (pSocket->m_nState < SOCKET_SPLASH)
	{
		// Account Handling Generation
		if (Nanny(pSocket, sInput))
		{
			// Nanny complete - ready to progress to the next step
			return;
		}
	}
	else
	{
		// Playing the Game - Handled by their GameWorld now
		if (pSocket->m_pWorld)
		{
			pSocket->m_pWorld->Interpret(pSocket, sInput);
			return;
		}
		else
		{
			Console("Interpret :: %d World/State conflict.", pSocket->GetSocket());
			return;
		}
	}

	if (pSocket->m_nState == SOCKET_CONNECTED)
	{
		if (sInput == "shutdown")
			Shutdown();
		else if (sInput == "who")
			Who(pSocket);
		else
			Chat(pSocket, sInput);
	}

	return;
}

// GameServer loop
// Checks for new connections
void GameServer::GameLoop()
{
	// Check for new connections
	list<vector<GameSocket*>::iterator> DeleteList;
	int nCount = 0;

	// Timers
	time_t TimerActor	 = 0;
	time_t TimerArea	 = 0;
	time_t TimerWorld	 = 0;
	time_t TimerWho		 = 0;
	time_t TimerCombat   = 0;


	time_t TimerStart = GameServer::Time();

	while (!m_bShutdown)
	{
		try
		{
			time_t Timer = GameServer::Time();


			// Check for new connections
			AcceptNew();

			// Delete any pending connections on delete list
			for (list<vector<GameSocket*>::iterator>::iterator it = DeleteList.begin(); it != DeleteList.end(); it++)
			{
				Console("Closing socket [%d]", (*(*it))->GetSocket());
				m_Sockets.erase(*it);
			}

			DeleteList.clear();

			// Handle connections
			for (vector<GameSocket*>::iterator it = m_Sockets.begin(); it != m_Sockets.end(); it++)
			{
				if ((*it)->GetSocket() < 0)
					continue;

				if ((*it)->m_bDelete)
					continue;

				if ((*it)->m_nState == SOCKET_ANSI)
				{
					(*it)->m_nState++;
				}

				if ((*it)->m_nState == SOCKET_CONNECTED)
				{
					// Temp
					(*it)->Send("                        &rW&Relcome &rt&Ro &rM&RicroMud\n\r\n\r");
					(*it)->Send("                          ");
					(*it)->MXPSendTag("<a 'http://www.micromud.org'>");
					(*it)->Send("www.micromud.org\n\r");
					(*it)->MXPSendTag("</a>");
					(*it)->Send("                       Running Build: %s\n\r\n\r", BUILD_VERSION);



					(*it)->Send("\n\rEnter your account name: ");
					(*it)->m_nState++;	// Progress to SOCKET_GREETING
				}

				if ((*it)->m_nState >= SOCKET_PLAYING)
				{
					// Handle idle timeout
					// After 10 minutes AFK
					// After 20 minutes disconnect
					if (difftime(GameServer::Time(), (*it)->m_tLastInput ) > 600.0 && (*it)->m_Variables["afk"] != "yes")
					{
						(*it)->m_Variables["afk"] = "yes";
						(*it)->Send("You are now Away From Keyboard [AFK]\n\r");
						(*it)->m_pPlayer->m_pRoom->Send((*it)->m_pPlayer, "%s is now AFK.\n\r", (*it)->m_pPlayer->m_sName.c_str());
					}

					if (difftime(GameServer::Time(), (*it)->m_tLastInput ) > 1200.0 && (*it)->m_Variables["afk"] != "yes")
					{
						(*it)->Send("The Emperor does not reward idle hands!\n\r");
						(*it)->Send("[Exceeded 20 minute idle period... Disconnecting]\n\r");
						Console("%s has been disconnected (Idle timeout)", (*it)->m_pPlayer->m_sName.c_str());
						(*it)->m_pPlayer->m_pRoom->RemActor((*it)->m_pPlayer);	// Remove from room
						(*it)->m_pPlayer->m_pRoom->Send("%s disappears from the room.\n\r", (*it)->m_pPlayer->m_sName.c_str());
						(*it)->m_pWorld->RemPlayer((*it)->m_pPlayer);			// Remove from world
						(*it)->Close();
						delete (*it);
						continue;
					}
				}

				string sInput;
				char input[MAXRECV];
				memset ( input, 0, MAXRECV+1 );

				if (!(*it)->Receive(input))
				{
					// Connection closed by other end so lets delete this entry
					(*it)->m_bDelete = true;
					DeleteList.push_back(it);
				}

				if (input[0] != '\0')
				{

					// Parse input for Telnet protocol
					sInput = (*it)->ReceiveNegotiation(input);

					if (sInput.data()[0] != '\0')
					{
						Interpret(*it, sInput);
						(*it)->m_tLastInput = GameServer::Time();

						if ((*it)->m_Variables["afk"] == "yes")
						{
							(*it)->Send("You are no longer AFK.\n\r");
							(*it)->m_pPlayer->m_pRoom->Send((*it)->m_pPlayer, "%s is no longer AFK.\n\r", (*it)->m_pPlayer->m_sName.c_str());
							(*it)->m_Variables["afk"] = "no";
						}
					}
				}

			}

			// Handle Updates
			time_t CurrentTimer = Timer - TimerStart;

			// Update Players online for website
			if ( (CurrentTimer % PULSE_WHO) == 0 && CurrentTimer != TimerWho)
			{
				ofstream outputFile("../public_html/online.txt");
				outputFile << NumPlayers();
				TimerWho = CurrentTimer;	// Last time updated
			}

			// Update Game Worlds
			if ( (CurrentTimer % PULSE_WORLD) == 0 && CurrentTimer != TimerWorld)
			{
				for (std::vector<GameWorld*>::iterator it = m_Worlds.begin(); it != m_Worlds.end(); it++)
					(*it)->Update();

				TimerWorld = CurrentTimer;
			}

			// Update Areas
			if ( (CurrentTimer % PULSE_AREA) == 0 && CurrentTimer != TimerArea)
			{
				for (std::vector<GameWorld*>::iterator it = m_Worlds.begin(); it != m_Worlds.end(); it++)
				{
					for (map<int,Area*>::iterator jt = (*it)->m_Areas.begin(); jt != (*it)->m_Areas.end(); jt++)
						(*jt).second->Update();
				}

				TimerArea = CurrentTimer;
			}

			// Update all players
			if ( (CurrentTimer % PULSE_ACTOR) == 0 && CurrentTimer != TimerActor)
			{
				for (std::vector<GameWorld*>::iterator it = m_Worlds.begin(); it != m_Worlds.end(); it++)
				{
					for (map<int, Player*>::iterator jt = (*it)->m_Players.begin(); jt != (*it)->m_Players.end(); jt++)
						(*jt).second->Update();
				}

				TimerActor = CurrentTimer;
			}

			// Update any PvP combat
			if ( (CurrentTimer % PULSE_COMBAT) == 0 && CurrentTimer != TimerCombat)
			{
				for (std::vector<GameWorld*>::iterator it = m_Worlds.begin(); it != m_Worlds.end(); it++)
					(*it)->CombatManager();

				TimerCombat = CurrentTimer;
			}

			nCount++;

		}
		catch (SocketException& e)
		{
			Console(":: [GameLoop] Exception was caught: %s [EXITING]", e.description().c_str());
		}

	}

}

// Set up the Server for receiving connections on a specified port and boot the database
void GameServer::Initialise(int port)
{
	try
	{
		// Set the game port
		m_nPort = port;

		// Create the server socket
		m_pServer = new GameSocket ( m_nPort );
		m_pServer->SetNonBlocking();
		m_nSocketId = m_pServer->GetSocket();

		// Loadup the database
		SqliteDbase::Get()->Open();

		// Load the GameWorlds
		if ( !LoadGameWorlds() )
			exit(1);
	}
	catch (SocketException& e)
	{
		Console(" :: [Initialise] Exception was caught: %s [EXITING]", e.description().c_str());
	}

}

bool GameServer::HasSocket(GameSocket & pSocket)
{
	for (vector<GameSocket*>::iterator it = m_Sockets.begin(); it != m_Sockets.end(); it++)
	{
		return true;
	}

	return false;
}

void GameServer::AcceptNew ( )
{
	// Use select to check for any waiting descriptors to serve
	fd_set readfds;
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 15;

	FD_ZERO(&readfds);
	FD_SET(m_pServer->GetSocket(), &readfds);

	::select(m_pServer->GetSocket()+1, &readfds, NULL, NULL, &timeout);

	if (FD_ISSET(m_pServer->GetSocket(), &readfds))
	{
		// There is a new connection waiting for us - lets accept it
		GameSocket * pSocket = new GameSocket();
		if (m_pServer->Accept(pSocket))
		{
			this->m_nSocketId++;
			pSocket->m_pServer = this;
			pSocket->SetNonBlocking();

			m_Sockets.push_back(pSocket);
			Console("New Connection [%d] %s", pSocket->GetSocket(), pSocket->Address().c_str());
			m_nSocketId++;

			pSocket->Negotiate();
		}
		else
		{
			delete pSocket;
		}
	}
}

//
// COMMANDS
//

void GameServer::Send(string sMsg)
{
	for (vector<GameSocket*>::iterator it = m_Sockets.begin(); it != m_Sockets.end(); it++)
	{
		(*it)->Send(sMsg);
	}
}

// Console - Display a message on the console
void GameServer::Console(const std::string &sMsg, ...)
{
    char buf[MAX_STRING_LENGTH*2];
    va_list args;

    va_start(args, sMsg);
    vsprintf(buf, sMsg.c_str(), args);
    va_end(args);

    string sEdit(buf);

    cout << StrTime() <<  "[GameServer] " << sEdit << endl;
    return;
}

void GameServer::Shutdown()
{
	Send("MicroMud server is shutting down...\n\r");
	m_bShutdown = true;

	for (vector<GameSocket*>::iterator it = m_Sockets.begin(); it != m_Sockets.end(); it++)
	{
		(*it)->Close();
	}

	m_pServer->Close();
}

void GameServer::Chat(GameSocket * pSocket, string sInput)
{
	for (vector<GameSocket*>::iterator it = m_Sockets.begin(); it != m_Sockets.end(); it++)
	{
		if ((*it)->GetSocket() < 0)
			continue;

		if (pSocket == *it)
			continue;

		(*it)->Send("%d: %s\n\r", pSocket->GetSocket(), sInput.c_str());
	}

	pSocket->Send("You: %s\n\r", sInput.c_str());
}

void GameServer::Who(GameSocket * pSocket)
{
	pSocket->Send("&rM&Ricro&rM&Rud &W:: &cWho's online\n\r");

	for (vector<GameSocket*>::iterator it = m_Sockets.begin(); it != m_Sockets.end(); it++)
	{
		if ((*it)->m_bDelete)
			continue;

		if ((*it)->m_nState != SOCKET_GREETING)
			continue;

		pSocket->Send("  %d\n\r", (*it)->GetSocket());
	}

	pSocket->Send("&rTotal Players&R: &c%d\n\r", m_Sockets.size());
}

int GameServer::NumConnected()
{
	int nCounter = 0;

	for (vector<GameSocket*>::iterator it = m_Sockets.begin(); it != m_Sockets.end(); it++)
	{
		if ((*it)->m_bDelete)
			continue;

		if ((*it)->m_nClient == CLIENT_WEB)
			continue;

		if ((*it)->m_nState != SOCKET_PLAYING)
			continue;

		nCounter++;
	}

	return nCounter;
}

// Load the GameWorlds in from their xml data file
// Taken from gameworlds.xml in the data folder
bool GameServer::LoadGameWorlds()
{
	XMLDocument doc;
	doc.LoadFile( "data/gameworlds.xml" );

	if (doc.Error())
	{
		doc.PrintError();
		return false;
	}

	XMLElement * pText = doc.FirstChildElement( "GameWorld" );

	// Parse the XML file for data
	while ( pText )
	{
		string sName;
		string sDesc;
		GameWorld * pWorld = NULL;

		for (XMLElement * pElement = pText->FirstChildElement(); pElement; pElement = pElement->NextSiblingElement())
		{
			if (StrEquals(pElement->Name(), "name"))
			{
				sName = pElement->GetText();

				// Load specific GameWorlds
				if (sName == "Shinobi")
					pWorld = new Shinobi();
				else
					pWorld = new GameWorld();
			}

			if (StrEquals(pElement->Name(), "desc"))
				sDesc = pElement->GetText();

			// Any entry in the config file that has a flag of config is loaded into the
			// variables map for later use
			if (StrEquals(pElement->Name(), "config"))
			{
				string sVariable = pElement->Attribute("name");
				string sType = pElement->Attribute("type");
				string sValue = pElement->GetText();

				if (sType == "int")
					pWorld->m_Variables->Set(sVariable, atoi(sValue.c_str()));
				else
					pWorld->m_Variables->Set(sVariable, sValue);

				Console("Config: %s %s", sVariable.c_str(), sValue.c_str());
			}

			// Import Player Attributes
			if (StrEquals(pElement->Name(), "attribute"))
			{
				string sId = pElement->Attribute("id");
				string sName = pElement->GetText();
				pWorld->m_Attributes[sId] = sName;
			}

			// Import colour strings
			if (StrEquals(pElement->Name(), "colour"))
			{
				string sR = pElement->Attribute("r");
				string sG = pElement->Attribute("g");
				string sB = pElement->Attribute("b");
				string sName = pElement->GetText();
				sR.append(sG);
				sR.append(sB);
				pWorld->m_Colours[sName] = sR;
				Console("Colour: %s %s", sName.c_str(), sR.c_str());
			}
		}

		// Set the name and the Desc
		pWorld->m_sDesc = sDesc;
		pWorld->m_sName = sName;


		// Load Attributes


		// Load Wear Locations TODO
		// Load Races TODO


		m_Worlds.push_back(pWorld);

		Console("%s Loaded.", pWorld->m_sName.c_str());
		pWorld->LoadAreas();
		pWorld->LoadCombatMessages();

		pText = pText->NextSiblingElement( "GameWorld" );
	}

	if (m_Worlds.size() <= 0)
	{
		Console(":: [LoadGameWorlds] No GameWorlds found.");
		return false;
	}

	Console("%d GameWorlds loaded.", m_Worlds.size());
	return true;
}

void GameServer::DisplayWorlds(GameSocket * pSocket)
{
	pSocket->Send("\n\rGameworlds&r:\n\r");
	int nCount = 1;
	for (std::vector<GameWorld*>::iterator it = m_Worlds.begin(); it != m_Worlds.end(); it++)
	{
		pSocket->Send("%2d - %s", nCount, (*it)->m_sName.c_str());

		if ((*it)->NumPlayers() > 0)
			pSocket->Send(" &g[&G%d Player%s online&g]", (*it)->NumPlayers(), (*it)->NumPlayers() > 1 ? "s" : "");

		pSocket->Send("\n\r");
		nCount++;
	}

	if (nCount == 1)
		pSocket->Send("&RError - No Gameworlds found.&w\n\r");
	else
		pSocket->Send("\n\rEnter the name/number of the Gameworld you wish to play: ");

	return;
}

// Determine colour settings, MXP support etc
void GameServer::DetermineProtocols (GameSocket * pSocket)
{
	pSocket->Send("\n\rDetermining Protocol support...\n\r");
	pSocket->Determine256();

	if (pSocket->m_Variables["256colour"] == "yes")
		pSocket->Send(" * &R2&G5&C6&w colours enabled.\n\r");
	else
		pSocket->Send(" * &RA&GN&CS&YI&w colour enabled.\n\r");

	if (pSocket->m_Negotiated[TELOPT_MSDP] == ACCEPTED)
		pSocket->Send(" * MSDP enabled.\n\r");

	if (pSocket->m_Negotiated[TELOPT_MSP] == ACCEPTED)
	{
		// Send a sound TODO
		pSocket->Send(" * MSP enabled.\n\r");
	}

	if (pSocket->m_Negotiated[TELOPT_MXP] == ACCEPTED)
	{
		pSocket->Send(" * ");
		pSocket->MXPSendTag("<u>");
		pSocket->Send("MXP enabled.\n\r");
		pSocket->MXPSendTag("</u>");
	}

	// Display detected client version
	if (!pSocket->m_Variables["client"].empty())
	{
		if (!pSocket->m_Variables["client_version"].empty())
			pSocket->Send(" * %s v%s detected\n\r", ToCapital(pSocket->m_Variables["client"]).c_str(), pSocket->m_Variables["client_version"].c_str());
		else
			pSocket->Send(" * %s detected\n\r", ToCapital(pSocket->m_Variables["client"]).c_str());
	}
}

// Nanny -- Named for good old times!
// Handles Account creation and log in for the GameServer
bool GameServer::Nanny(GameSocket * pSocket, string sInput)
{
	// They entered an account name
	if (!pSocket->m_pAccount)
	{
		// Check it exists
		stringstream sSql;
		sSql << "select accountid from accountdata where name like '" << sInput << "'";
		int nId = SqliteDbase::Get()->ExecScalar(sSql.str().c_str());

		if (nId == 0)
		{
			// Doesn't exist
			pSocket->Send("Do you wish to create a New Account for %s? (Y/N): ", sInput.c_str());
			pSocket->m_pAccount = new Account();
			pSocket->m_pAccount->m_sName = sInput;
			pSocket->m_nState = SOCKET_CONFIRM_NAME;
		}
		else
		{

			pSocket->Send("Password: ");
			pSocket->m_nState = SOCKET_PASSWORD;
			pSocket->m_pAccount = new Account();
			pSocket->m_pAccount->m_sName = sInput;
			//pSocket->m_pProtocol->DisableEcho(pSocket, true);	// Hide password input
		}
	}
	else
	{
		// Account now loaded, switch on state
		if (pSocket->m_nState == SOCKET_CONFIRM_NAME)
		{
			if (sInput == "y")
			{
				pSocket->Send("New Account created.\n\r");
				Console("%s has created a new Account.", pSocket->m_pAccount->m_sName.c_str());
				pSocket->Send("Enter a password for this account: ");
				pSocket->m_nState = SOCKET_ENTER_PASSWORD;
				return false;
			}
			else if (sInput == "n")
			{
				delete pSocket->m_pAccount;
				pSocket->m_pAccount = NULL;
				pSocket->m_nState = SOCKET_GREETING;
				return false;
			}
			else
			{
				pSocket->Send("Invalid input, enter Y to create an Account or N to chose another.\n\r");
				return false;
			}

		}
		if (pSocket->m_nState == SOCKET_PASSWORD)
		{
			// Compare the Account password to this password
			//pSocket->m_pProtocol->DisableEcho(pSocket, false);
			string sPass = SqliteDbase::Get()->GetString("accountdata", "password", "name", pSocket->m_pAccount->m_sName);

			string sResult(crypt(sInput.c_str(), pSocket->m_pAccount->m_sName.c_str()));

			if (sResult == sPass)
			{
				pSocket->Send("&GPassword correct.\n\r");
				Console("%s has connected.", pSocket->m_pAccount->m_sName.c_str());
				pSocket->m_nState = SOCKET_GAMEWORLD;


				DetermineProtocols(pSocket);
				DisplayWorlds(pSocket);

				// Load account data from database
				pSocket->m_pAccount->m_sPass = sPass;
				pSocket->m_pAccount->m_sEmail = SqliteDbase::Get()->GetString("accountdata", "email", "name", pSocket->m_pAccount->m_sName);
				pSocket->m_nId = SqliteDbase::Get()->GetInt("accountdata", "accountid", "name", pSocket->m_pAccount->m_sName);
				return false;
			}
			else
			{
				Console ("Incorrect password attempt for %s", pSocket->m_pAccount->m_sName.c_str());
				pSocket->Send("&RIncorrect Password.\n\r");
				pSocket->Close();
				delete pSocket;
				return false;
			}

		}
		else if (pSocket->m_nState == SOCKET_ENTER_PASSWORD)
		{
			// Validate password entry
			if (sInput.length() < 6)
			{
				pSocket->Send("Passwords must be longer than 6 characters, try again: ");
				return false;
			}
			else
			{
				pSocket->Send("Password set.\n\r");
				pSocket->m_pAccount->m_sPass = crypt(sInput.c_str(), pSocket->m_pAccount->m_sName.c_str());
				pSocket->Send("Enter an email address for this account: ");
				pSocket->m_nState = SOCKET_EMAIL;
				return false;
			}

		}
		else if (pSocket->m_nState == SOCKET_EMAIL)
		{
			// Validate email entry
			pSocket->Send("Email set.\n\r");
			pSocket->m_pAccount->m_sEmail = sInput;
			pSocket->m_nState = SOCKET_GAMEWORLD;

			// Put all this information into the Database
			stringstream sSQL;
			sSQL << "insert into accountdata (name, password, email) values ('" << pSocket->m_pAccount->m_sName << "', '" << pSocket->m_pAccount->m_sPass << "', '" << pSocket->m_pAccount->m_sEmail << "');";
			SqliteDbase::Get()->ExecDML(sSQL.str().c_str());
			// Get Account Id back
			pSocket->m_pAccount->m_nId = SqliteDbase::Get()->GetInt("accountdata", "accountid", "name", pSocket->m_pAccount->m_sName);

			// Display Protocol support
			pSocket->Send("Determining Protocol support.\n\r");
			DetermineProtocols(pSocket);
			// Display Gameworlds
			DisplayWorlds(pSocket);

			return false;
		}
		else if (pSocket->m_nState == SOCKET_GAMEWORLD)
		{
			// Validate entry for GameWorld
			// First, assume its the name of a GameWorld
			int nCount = 1;
			GameWorld * pWorld = NULL;

			for (std::vector<GameWorld*>::iterator it = m_Worlds.begin(); it != m_Worlds.end(); it++)
				if ((*it)->m_sName == sInput)
					pWorld = (*it);

			if (!pWorld)
			{
				for (std::vector<GameWorld*>::iterator it = m_Worlds.begin(); it != m_Worlds.end(); it++)
				{
					if (nCount == atoi(sInput.c_str()))
					{
						pWorld = (*it);
						break;
					}

					nCount++;
				}

			}

			if (!pWorld)
			{
				pSocket->Send("That is not a valid Gameworld name or number, try again: ");
				return false;
			}

			// Display Splash menu for the given World
			pWorld->Splash(pSocket);
			pSocket->m_pWorld = pWorld;
			Console("%s has entered %s.", pSocket->m_pAccount->m_sName.c_str(), pWorld->m_sName.c_str());

			pSocket->m_nState = SOCKET_SPLASH;
			return true;
		}

	}

	return false;
}

// Check if this string is a Protocol that we are supporting
bool GameServer::IsProtocol (string sStr)
{
	bool bFound = false;

	for (list<char>::iterator it = m_Protocols.begin(); it != m_Protocols.end(); it++)
	{
		if (Protocol::ConvertTelnetToChar(sStr) == (*it))
			return true;
	}

	return bFound;
}

// As above, however, checking if this char is a protocol we support
bool GameServer::IsProtocol (char cStr)
{
	bool bFound = false;

	for (list<char>::iterator it = m_Protocols.begin(); it != m_Protocols.end(); it++)
	{
		if (cStr == (*it))
			return true;
	}

	return bFound;
}

int GameServer::NumPlayers()
{
	int nNum = 0;

	for (vector<GameWorld*>::iterator it = m_Worlds.begin(); it != m_Worlds.end(); it++)
		nNum += (*it)->NumPlayers();

	return nNum;
}

// Send MSSP string
void GameServer::SendMSSP (GameSocket * pSocket)
{
	char MSSP[MAX_MSSP_BUFFER];
	stringstream ssStr;

	//ssStr << this->NumPlayers();

	// Kick off with IAC SB MSSP
	sprintf( MSSP, "%c%c%c", (char)IAC, (char)SB, TELOPT_MSSP);

	// Add all the variables
	// Required
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "NAME", 			MSSP_VAL, "Shinobi");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "PLAYERS", 		MSSP_VAL, ssStr.str().c_str());
	// TODO Implement this
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "UPTIME", 			MSSP_VAL, "0");
	// Generic
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "CRAWL DELAY", 	MSSP_VAL, "-1");
	// Extra
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "HOSTNAME",		MSSP_VAL, "micromud.mudhosting.net");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "PORT",			MSSP_VAL, "6660");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "CODEBASE",		MSSP_VAL, "MicroMud");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "CONTACT",	 		MSSP_VAL, "owen@micromud.org");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "CREATED",	 		MSSP_VAL, "2012");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "IP",	 			MSSP_VAL, "66.85.147.90");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "LANGUAGE",		MSSP_VAL, "English");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "LOCATION",		MSSP_VAL, "UK");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "MINIMUM AGE",		MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "WEBSITE",	 		MSSP_VAL, "www.micromud.org");

	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "FAMILY",	 		MSSP_VAL, "Custom");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "GENRE",	 		MSSP_VAL, "");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "GAMEPLAY",		MSSP_VAL, "Player versus Player");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "STATUS",	 		MSSP_VAL, "Alpha");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "GAMESYSTEM",		MSSP_VAL, "Custom");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "INTERMUD",		MSSP_VAL, "");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "SUBGENRE",		MSSP_VAL, "");

	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "AREAS",	 		MSSP_VAL, "2");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "HELPFILES",		MSSP_VAL, "100");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "MOBILES",	 		MSSP_VAL, "15");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "OBJECTS",	 		MSSP_VAL, "26");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "ROOMS",	 		MSSP_VAL, "25");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "CLASSES",	 		MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "LEVELS",	 		MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "RACES",	 		MSSP_VAL, "1");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "SKILLS",	 		MSSP_VAL, "0");

	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "ANSI",	 		MSSP_VAL, "1");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "GMCP",	 		MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "MCCP",	 		MSSP_VAL, "1");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "MCP",	 			MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "MSDP",	 		MSSP_VAL, "1");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "MSP",	 			MSSP_VAL, "1");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "MXP",	 			MSSP_VAL, "1");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "PUEBLO",	 		MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "UTF-8",	 		MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "VT100",	 		MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "XTERM 256 COLORS",MSSP_VAL, "1");

	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "PAY TO PLAY", 	MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "PAY FOR PERKS", 	MSSP_VAL, "0");

	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "HIRING BUILDERS",MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "HIRING CODERS", 	MSSP_VAL, "0");

	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "ADULT MATERIAL", 	MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "MULTICLASSING", 	MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "NEWBIE FRIENDLY",MSSP_VAL, "1");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "PLAYER CITIES", 	MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "PLAYER CLANS", 	MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "PLAYER CRAFTING",MSSP_VAL, "1");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "PLAYER GUILDS", 	MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "EQUIPMENT SYSTEM",MSSP_VAL, "1");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "MULTIPLAYING", 	MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "PLAYERKILLING", 	MSSP_VAL, "1");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "QUEST SYSTEM", 	MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "ROLEPLAYING", 	MSSP_VAL, "0");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "TRAINING SYSTEM",MSSP_VAL, "1");
	sprintf( MSSP, "%c%s%c%s", MSSP_VAR, "WORLD ORIGINALITY",MSSP_VAL, "1");

	// Finish the text with IAC SE
	sprintf( MSSP, "%c%c", (char)IAC, (char) SE);
	pSocket->Send("%s\0", MSSP);
}

string GameServer::StrTime()
{
	stringstream ssStr;

	time_t rawtime = GameServer::Time();
	struct tm * timeinfo;
	timeinfo = localtime ( &rawtime );

	char * sTime = asctime(timeinfo);
	sTime[strlen(sTime)-1] = '\0';

	ssStr << "[" << sTime << "] ";
	return ssStr.str();
}

time_t GameServer::Time()
{
	time_t rawtime;

	time ( &rawtime );

	return rawtime;
}
