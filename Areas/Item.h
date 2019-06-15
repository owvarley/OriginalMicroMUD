/*
 * Item.h
 *
 *  Created on: Mar 26, 2012
 *      Author: Gnarls
 */

#ifndef ITEM_H_
#define ITEM_H_

#include <map>
#include <list>
#include <string>

using namespace std;

class VariableManager;

class Item
{
	public:
		Item();
		virtual ~Item();

		void						Load();									// Load an item's details from the database
		bool						HasFlag(string sVar);					// Check if has flag
		void						AddFlag(string sVar);					// Add a flag
		void						RemFlag(string sVar);					// Remove a flag
		void						Update();								// Update the item

		// Variables
		int							m_nId;			// Item ID
		string						m_sName;		// Item name
		map<string, int>			m_Flags;		// Item flags
		VariableManager *			m_Variables;	// Variables
		list<Item*>					m_Contents;		// For Containers

};

#endif /* ITEM_H_ */
