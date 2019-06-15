/*
 * Main.h
 *
 *  Created on: Mar 15, 2012
 *      Author: Gnarls
 */

#ifndef MAIN_H_
#define MAIN_H_

#include <vector>
#include <map>
#include <list>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>

using namespace std;

#define MAX_INPUT_LENGTH 1024
#define MAX_STRING_LENGTH 4096


#ifndef MUD_NAME
#define MUD_NAME						"MicroMud"
#endif

#define BUILD_VERSION					"v0.15a"

// Defines for Telnet Protocol handling
#define MAX_PROTOCOL_BUFFER            2048
#define MAX_VARIABLE_LENGTH            4096
#define MAX_OUTPUT_BUFFER              8192
#define MAX_MSSP_BUFFER                4096

// Handshake defines
#define NOTHING							0
#define SEND                           1
#define ACCEPTED                       2
#define REJECTED                       3

// Telnet MUD Protocol option defines
#define TELOPT_CHARSET                 42
#define TELOPT_MSDP                    69
#define TELOPT_MSSP                    70
#define TELOPT_MCCP                    86 // V2
#define TELOPT_MSP                     90
#define TELOPT_MXP                     91
#define TELOPT_ATCP                    200

// MSDP Defines
#define MSDP_VAR 						1
#define MSDP_VAL 						2
#define MSDP_TABLE_OPEN 				3
#define MSDP_TABLE_CLOSE 				4
#define MSDP_ARRAY_OPEN 				5
#define MSDP_ARRAY_CLOSE 				6

// MSSP Defines
#define MSSP_VAR 						1
#define MSSP_VAL 						2
#define MAX_MSSP_BUFFER                4096

// MXP Defines
#define ESC "\x1B"
#define MXP_BEG "\x03"
#define MXP_END "\x04"
#define MXP_AMP "\x05"
#define MXPTAG (arg) MXP_BEG arg MXP_END
#define MXPMODE (arg) ESC "[" #arg "z"

#define CLIENT_TELNET 0
#define CLIENT_GUI 1
#define CLIENT_MOBILE 2
#define CLIENT_WEB 3

//
#define _ROOM			1
#define _ACTOR			2
#define _WORLD			3

// Inheritance for Player/Npc/Actor
#define _PLAYER			0
#define _NPC			1

// Update timers
#define PULSE_ACTOR		2		// Every 10 seconds
#define PULSE_WORLD		1		// Every 1 second
#define PULSE_AREA		60		// Every 1 minutes
#define PULSE_COMBAT	6 		// Every 6 seconds
#define PULSE_WHO		6		// Every 1 minute

// SEX
#define SEX_MALE		0
#define SEX_FEMALE		1
#define SEX_NEUTRAL    2

// Affects
#define AFF_NONE		1
#define AFF_ATTR_1		2
#define AFF_ATTR_2		3
#define AFF_ATTR_3		4
#define AFF_ATTR_4		5
#define AFF_ATTR_5		6

// Act scopes
#define ACT_CH			0
#define ACT_VICTIM		1
#define ACT_ROOM		2

// Combat stances
#define COMBAT_OFFENSIVE 0
#define COMBAT_DEFENSIVE 1

using namespace std;

class Actor;
class Item;

class Main
{
	public:
		Main();
		virtual ~Main();

		static int rand_range(int nFrom, int nTo);
		static string CardinalDirection(int nX, int nY, int nZ, int nToX, int nToY, int nToZ);
		static string ReverseDirection(string sDir);
		static bool IsCardinal(string sDir);
		static string ConvertCardinal(string sDir);

};

// Global game functions

// String manipulation
bool StrEquals(string a, string b);
bool StrContains(string a, string b);

string ToUpper(string str);
string ToLower(string str);
string ToCapital(string str);
string ToCapitals(string str);
string StrReplace(string &str, string find, string replacement);
string ActReplace(int nScope, Actor * pActor, Actor * pVictim, Item * pItem, Item * pItem2, string sStr, Actor * pViewer);


#endif /* MAIN_H_ */
