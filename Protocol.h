/*
 * Protocol.h
 *
 *  Created on: Aug 28, 2012
 *      Author: owen
 *        Note: Protocol support is heavily based upon KaVir's Protocol Handler, modified for MicroMud
 *            : http://www.mudbytes.net/file-2811
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include <string>
#include "Main.h"

// Set to 1 to view all Protocol negotiation in the console
#define PROTOCOL_DEBUG 1

using namespace std;

class GameSocket;


class Protocol
{
	public:
		Protocol();
		virtual ~Protocol();

		static char    ConvertTelnetToChar (string sEntry);
		static string  ConvertMSDP (char sEntry);
		static bool   IsMSDP (char sEntry);
		static string  ConvertTelnetToString (char sEntry, string sScope);
		static bool	IsHandShake (char sEntry);
		static bool 	ConvertDoWill (char sWord);
		static string  ReplaceMSDP (string sStr);

};

#endif /* PROTOCOL_H_ */
