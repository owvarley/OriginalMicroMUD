/*
 * Variable.h
 *
 *  Created on: Aug 26, 2012
 *      Author: owen
 */

#ifndef VARIABLE_H_
#define VARIABLE_H_

#include <string>
#include <map>

using namespace std;

class Variable
{
	public:
		Variable();
		virtual ~Variable();
		string 		Get();
		int			GetI();
		void		Set(string sValue);
		void		Set(int nValue);

	public:
		string 		m_sValue;
		int			m_nValue;
		bool		m_bInt;

};

class VariableManager
{
	public:
		VariableManager();
		virtual ~VariableManager();

		// Variable functions
		void						Set(string sVariable, int nValue);		// Set an int value to a variable
		void						Set(string sVariable, string sValue);	// Set a string value to a variable
		string						Var(string sVar);						// Return a variable as a string
		int							VarI(string sVar);						// Return the int value of a variable
		bool						HasVar(string sVar);					// Does this actor have a given variable
		void						AddToVar(string sVar, int nAmount);	// Increment an int variable
		void						AddVar(string sVar, int nValue);		// Add a new variable
		void						AddVar(string sVar, string sValue);		// Add a new variable string
		void						RemVar(string sVar);					// Remove a variable
			// Remove a variable

	public:
		map<string, Variable*>		m_Variables;

};

#endif /* VARIABLE_H_ */
