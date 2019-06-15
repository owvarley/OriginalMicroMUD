/*
 * Affect.h
 *
 *  Created on: Sep 11, 2012
 *      Author: owen
 */

#ifndef AFFECT_H_
#define AFFECT_H_

#include <string>

using namespace std;

class Affect
{

	public:
		Affect();
		virtual ~Affect();

		// Variables
		string				m_sName;		// Name of affect
		int					m_nLocation;	// What does it affect
		int					m_nModifier;	// How much does it modify this location by
		int					m_nSeconds;		// Number of seconds left
		bool				m_bPermanent;	// Does this affect decay?

};

#endif /* AFFECT_H_ */
