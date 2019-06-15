/*
 * Account.h
 *
 *  Created on: Mar 23, 2012
 *      Author: Gnarls
 */

#ifndef ACCOUNT_H_
#define ACCOUNT_H_

#include <string>

using namespace std;

class Account
{
	public:
		Account();
		virtual ~Account();

		int					m_nId;
		string				m_sName;
		string				m_sEmail;
		string				m_sPass;

};

#endif /* ACCOUNT_H_ */
