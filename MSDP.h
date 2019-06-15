/*
 * MSDP.h
 *
 *  Created on: Sep 2, 2012
 *      Author: owen
 */

#ifndef MSDP_H_
#define MSDP_H_

#include <string>

using namespace std;

class MSDP
{

	public:
		MSDP();
		virtual ~MSDP();

		// Functions

		// Variables
		string			m_sVariable;
		string			m_sValue;
		bool			m_bReport;
		bool			m_bChanged;
};

#endif /* MSDP_H_ */
