/*
 * Exit.h
 *
 *  Created on: Mar 26, 2012
 *      Author: Gnarls
 */

#ifndef EXIT_H_
#define EXIT_H_

#include <map>
#include <string>

using namespace std;

class Exit
{
	public:
		Exit();
		virtual ~Exit();

	// Variables
		int						m_nId;
		int						m_nTo;
		string					m_sDir;
		map<string, int>		m_Flags;

};

#endif /* EXIT_H_ */
