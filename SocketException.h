/*
 * SocketException.h
 *
 *  Created on: Mar 17, 2012
 *      Author: Gnarls
 */

#ifndef SOCKETEXCEPTION_H_
#define SOCKETEXCEPTION_H_

// SocketException class
#include <string>

class SocketException
{
	public:
		SocketException ( std::string sString ) : m_s ( sString ) {};
		~SocketException (){};

		std::string description() { return m_s; }

	private:

		std::string m_s;
};

#endif /* SOCKETEXCEPTION_H_ */
