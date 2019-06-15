/*
 * Main.cpp
 *
 *  Created on: Mar 15, 2012
 *      Author: Gnarls
 */

#include "GameServer.h"
#include "GameWorld.h"
#include "Main.h"
#include <stdlib.h>
#include "SocketException.h"
#include "Actor.h"
#include "Areas/Item.h"
#include <algorithm>
#include "Libraries/MTRandint32.h"

#define STDIN 0

using namespace std;

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		std::cout << "[Main] Error - no port provided.\n";
		exit(1);
	}

	// Create a new Game Server object
	GameServer * pGameServer = new GameServer();

	// Initialise the Server
	pGameServer->Initialise(atoi(argv[1]));

	// Start the Game Loop
	pGameServer->GameLoop();

	// Normal shutdown
	exit(0);

	return 0;
}

int Main::rand_range(int nFrom, int nTo)
{
	// Seed
	MTRand_int32 irand;

	if (nTo < nFrom)
		return nFrom;

	if (nTo == nFrom)
		return nTo;

	int nRaw = irand();

	int nPos = abs(nRaw);

	int nRand = (nPos % nTo) + nFrom;

	if (nRand < nFrom)
		return nFrom;

	if (nRand > nTo)
		return nTo;

	return nRand;
}

// Work out the string description for a link between two sets of Coordinates.
// Positive X == East, Negative X == West
// Positive Y == North, Negative Y == South
string Main::CardinalDirection(int nX, int nY, int nZ, int nToX, int nToY, int nToZ)
{
	string sDir;
	string sDir2;

	if (nToX > nX)
		sDir = "east";
	else if (nToX < nX)
		sDir = "west";

	if (nToY > nY)
		sDir2 = "north";
	else if (nToY < nY)
		sDir2 = "south";

	if (nToZ > nZ)
		sDir = "up";
	else if (nToZ < nZ)
		sDir = "down";

	if (!sDir.empty() && !sDir2.empty())
		return sDir2.append(sDir);

	if (!sDir.empty())
		return sDir;

	if (!sDir2.empty())
		return sDir2;

	return "No Name";
}

string Main::ReverseDirection(std::string sDir)
{
	if (sDir == "north")
		return "south";
	if (sDir == "south")
		return "north";
	if (sDir == "west")
		return "east";
	if (sDir == "east")
		return "west";
	if (sDir == "up")
		return "down";
	if (sDir == "down")
		return "up";
	if (sDir == "northwest")
		return "southeast";
	if (sDir == "southeast")
		return "northwest";
	if (sDir == "northeast")
		return "southwest";
	if (sDir == "southwest")
		return "northeast";

	// Not a cardinal, so just return the original name
	return sDir;
}

bool Main::IsCardinal(string sDir)
{
	if (sDir == "north")
		return true;
	if (sDir == "south")
		return true;
	if (sDir == "west")
		return true;
	if (sDir == "east")
		return true;
	if (sDir == "up")
		return true;
	if (sDir == "down")
		return true;
	if (sDir == "northwest")
		return true;
	if (sDir == "southeast")
		return true;
	if (sDir == "northeast")
		return true;
	if (sDir == "southwest")
		return true;
	if (sDir == "nw")
		return true;
	if (sDir == "ne")
		return true;
	if (sDir == "sw")
		return true;
	if (sDir == "se")
		return true;

	return false;
}

// Converts short cuts for exits to their full name
string Main::ConvertCardinal (string sDir)
{
	if (sDir == "e")
		return "east";
	if (sDir == "w")
		return "west";
	if (sDir == "n")
		return "north";
	if (sDir == "s")
		return "south";
	if (sDir == "ne")
		return "northeast";
	if (sDir == "se")
		return "southeast";
	if (sDir == "nw")
		return "northwest";
	if (sDir == "sw")
		return "southwest";
	if (sDir == "u")
		return "up";
	if (sDir == "d")
		return "down";

	return sDir;
}

// Global Functions

// String manipulations

// Convert to lower text
string ToLower(string str)
{
	string str2(str);
	std::transform(str2.begin(), str2.end(), str2.begin(), ::tolower);
	return str2;
}

// Convert to upper text
string ToUpper(string str)
{
	string str2(str);
	std::transform(str2.begin(), str2.end(), str2.begin(), ::toupper);
	return str2;
}

// Capitalise the first letter
string ToCapital(string str)
{
	// Ignore any colour codes
	unsigned int nCount = 0;

	string str2(str);

	while (nCount < str2.length())
	{
		// If we find a colour code, add 2, this will skip the & sign and the following colour char
		if (str2.at(nCount) == '&')
		{
			nCount += 2;
			continue;
		}
		else
		{
			// We should never get to a letter indicating a colour code due to the code above, hence
			// it is safe now to assume we have our first letter and now we can capitalise it
			std::transform(str2.begin()+nCount, str2.begin()+(nCount+1), str2.begin()+nCount, ::toupper);
			return str2;
		}

		nCount++;
	}

	return str2;
}

// Capitalise every first letter of every word
string ToCapitals(string str)
{
	// Ignore any colour codes
	unsigned int nCount = 0;

	string str2(str);

	bool bFirst = true;

	while (nCount < str2.length())
	{
		// If we find a colour code, add 2, this will skip the & sign and the following colour char
		if (str2.at(nCount) == '&')
		{
			nCount += 2;
			continue;
		}
		else
		{
			// We should never get to a letter indicating a colour code due to the code above, hence
			// it is safe now to assume we have our first letter and now we can capitalise it
			if (bFirst)
			{
				std::transform(str2.begin()+nCount, str2.begin()+(nCount+1), str2.begin()+nCount, ::toupper);
				bFirst = false;
			}
		}

		if (str2[nCount] == ' ')
			bFirst = true;

		nCount++;
	}

	return str2;
}

// Replace tags for a string but providing a viewer who will be used for perception purposes
// Type == 0 -- Replace any instances of $n with You
// Type == 1 -- Replace any instances of $N with You
// Type == -1 - Replace any instances of $n or $N with their name
string ActReplace ( int nScope, Actor * pActor, Actor * pVictim, Item * pItem, Item * pItem2, string thestring, Actor * pViewer)
{
	if (pActor)
	{
		if (nScope == ACT_CH)
		{
			thestring = StrReplace(thestring, "$n's", "your");
			thestring = StrReplace(thestring, "$n", "you");
		}

		if (pViewer)
			thestring = StrReplace(thestring, "$n", pViewer->Perception(pActor));

	}

	if (pVictim)
	{
		// If there is no Victim then there should be no instance of $N to replace!
		if (nScope == ACT_VICTIM)
		{
			thestring = StrReplace(thestring, "$N's", "your");
			thestring = StrReplace(thestring, "$N", "you");
		}

		// If there is a Victim and a Viewer, convert the Victim's name into what the Viewer sees it to be
		if (pViewer)
			thestring = StrReplace(thestring, "$N", pViewer->Perception(pVictim));
	}


	if (pActor && pActor->m_Variables->VarI("sex") == SEX_MALE && nScope != ACT_CH)
	{
		thestring = StrReplace(thestring, "$e", "he");
		thestring = StrReplace(thestring, "$m", "him");
		thestring = StrReplace(thestring, "$s", "his");
	}
	else if (pActor && pActor->m_Variables->VarI("sex") == SEX_FEMALE && nScope != ACT_CH)
	{
		thestring = StrReplace(thestring, "$e", "she");
		thestring = StrReplace(thestring, "$m", "her");
		thestring = StrReplace(thestring, "$s", "her");
	}
	else if (pVictim && pVictim->m_Variables->VarI("sex") == SEX_NEUTRAL && nScope != ACT_CH)
	{
		thestring = StrReplace(thestring, "$e", "it");
		thestring = StrReplace(thestring, "$m", "its");
		thestring = StrReplace(thestring, "$s", "its");
	}

	if (pItem)
		thestring = StrReplace(thestring, "$o", pItem->m_sName.c_str());

	if (pVictim && pVictim->m_Variables->VarI("sex") == SEX_MALE && nScope != ACT_VICTIM)
	{
		thestring = StrReplace(thestring, "$E", "he");
		thestring = StrReplace(thestring, "$M", "him");
		thestring = StrReplace(thestring, "$S", "his");
	}
	else if (pVictim && pVictim->m_Variables->VarI("sex") == SEX_FEMALE && nScope != ACT_VICTIM)
	{
		thestring = StrReplace(thestring, "$E", "she");
		thestring = StrReplace(thestring, "$M", "her");
		thestring = StrReplace(thestring, "$S", "her");
	}
	else if (pVictim && pVictim->m_Variables->VarI("sex") == SEX_NEUTRAL && nScope != ACT_VICTIM)
	{
		thestring = StrReplace(thestring, "$E", "it");
		thestring = StrReplace(thestring, "$M", "its");
		thestring = StrReplace(thestring, "$S", "its");
	}

	if (nScope == ACT_CH)
	{
		thestring = StrReplace(thestring, "$z", "");
		thestring = StrReplace(thestring, "$Z", "es");
		thestring = StrReplace(thestring, "$p", "");
		thestring = StrReplace(thestring, "$P", "s");
		thestring = StrReplace(thestring, "$g", "");
		thestring = StrReplace(thestring, "$G", "ing");
		thestring = StrReplace(thestring, "$y", "y");
		thestring = StrReplace(thestring, "$Y", "ies");
		thestring = StrReplace(thestring, "$a", "are");
		thestring = StrReplace(thestring, "$A", "is");
	}
	else if (nScope == ACT_VICTIM)
	{
		thestring = StrReplace(thestring, "$z", "es");
		thestring = StrReplace(thestring, "$Z", "");
		thestring = StrReplace(thestring, "$p", "s");
		thestring = StrReplace(thestring, "$P", "");
		thestring = StrReplace(thestring, "$g", "ing");
		thestring = StrReplace(thestring, "$G", "");
		thestring = StrReplace(thestring, "$y", "ies");
		thestring = StrReplace(thestring, "$Y", "y");
		thestring = StrReplace(thestring, "$a", "is");
		thestring = StrReplace(thestring, "$A", "are");
	}
	else
	{
		thestring = StrReplace(thestring, "$z", "es");
		thestring = StrReplace(thestring, "$z", "es");
		thestring = StrReplace(thestring, "$p", "s");
		thestring = StrReplace(thestring, "$P", "s");
		thestring = StrReplace(thestring, "$g", "ing");
		thestring = StrReplace(thestring, "$G", "ing");
		thestring = StrReplace(thestring, "$y", "ies");
		thestring = StrReplace(thestring, "$Y", "ies");
		thestring = StrReplace(thestring, "$a", "is");
		thestring = StrReplace(thestring, "$A", "is");
	}

	thestring = StrReplace(thestring, "$e", "you");
	thestring = StrReplace(thestring, "$m", "you");
	thestring = StrReplace(thestring, "$s", "your");
	thestring = StrReplace(thestring, "$E", "you");
	thestring = StrReplace(thestring, "$M", "you");
	thestring = StrReplace(thestring, "$S", "your");

	if (pItem2)
		thestring = StrReplace(thestring, "$O", pItem2->m_sName.c_str());

	return thestring;
}


// Does string A equal to string B?
// Returns true if they do, false if not!
bool StrEquals(string a, string b)
{
	if (ToLower(a) == ToLower(b))
		return true;
	else
		return false;
}

// Does string A contain string B?
// Returns true if the string does contain B, false if not
bool StrContains(string a, string b)
{
	size_t found;

	if ( (found = (ToLower(a).find(ToLower(b)))) != string::npos)
		return true;
	else
		return false;
}

// Replace the FIND string within str with REPLACEMENT string
string StrReplace(string &str, string find, string replacement)
{
	size_t found = str.find(find);
	while (found != string::npos)
	{
		str.replace(found, find.length(), replacement);
		found = str.find(find);
	}

	return str;
}
