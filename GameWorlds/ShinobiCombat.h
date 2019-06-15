/*
 * ShinobiCombat.h
 *
 *  Created on: Sep 19, 2012
 *      Author: owen
 */

#ifndef SHINOBICOMBAT_H_
#define SHINOBICOMBAT_H_

#include "../Combat.h"

using namespace std;

class ShinobiCombat: public Combat
{
	public:
		ShinobiCombat();
		virtual ~ShinobiCombat();


		void				Update();
		void				End();

};


#endif /* SHINOBICOMBAT_H_ */
