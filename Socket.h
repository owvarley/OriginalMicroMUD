/*
 * Socket.h
 *
 *  Created on: Mar 17, 2012
 *      Author: Gnarls
 */

#ifndef SOCKET_H_
#define SOCKET_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>

const int MAXHOSTNAME = 200;
const int MAXCONNECTIONS = 5;
const int MAXRECV = 1000;

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

class Socket
{

	public:
		Socket();
		virtual ~Socket();

		// Server initialization
		bool create();
		bool bind ( const int port );
		bool listen() const;
		bool accept ( Socket* ) const;
		bool select ( Socket * nSock, int nId, fd_set * fIn, fd_set * fOut, fd_set * fErr, timeval * tTime ) const;
		int  close ();
		bool shutdown (int type);

		// Client initialization
		bool connect ( const std::string host, const int port );

		// Data Transmission
		bool send ( const std::string ) const;
		//int recv ( std::string& ) const;
		int recv ( char input[MAXRECV] ) const;

		void set_non_blocking ( const bool );
		bool is_valid() const { return m_sock != -1; }

	protected:
		int 			m_sock;
		sockaddr_in 	m_addr;

		friend class GameSocket;
		friend class GameServer;

};


#endif

