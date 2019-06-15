/*
 * SqliteDbase.h
 *
 *  Created on: Mar 23, 2012
 *      Author: Gnarls
 */

#ifndef SQLITEDBASE_H_
#define SQLITEDBASE_H_

#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include "Libraries/CppSQLite3.h"
#include "Main.h"


class SqliteDbase
{
	public:
		SqliteDbase() { init(); };
		virtual 		~SqliteDbase() { db->close(); delete db; };
		virtual void 	init() { db = NULL; };
		static SqliteDbase * Get();

		char 			buf[MAX_STRING_LENGTH];
		char 			dbname[20];

	protected:
		CppSQLite3DB * 	db;
		CppSQLite3Query lastquery;
		CppSQLite3Buffer bufSQL;

	public:
		bool 			Open();
		void 			Close();
		CppSQLite3Query GetLastQuery() { return lastquery; };
		CppSQLite3Table GetTable( CppSQLite3Buffer bufSQL );
		CppSQLite3Query ExecQuery(const char* szSQL);
		int 			ExecDML(const char* szSQL);
		int 			ExecScalar(const char* szSQL);
		string 			ExecString (const char* szSQL );
		bool			ExecExists(const char* szSQL);
		bool			RecordExists(std::string table, std::string field, std::string filter, std::string sValue);
		bool			RecordExists(std::string table, std::string field, std::string filter, int nValue);
		int				GetInt(std::string table, std::string field, std::string filter, int nValue);
		int				GetInt(std::string table, std::string field, std::string filter, std::string sValue);
		int				GetInt(std::string query, ...);
		string			GetString(std::string table, std::string field, std::string filter, int nValue);
		string			GetString(std::string table, std::string field, std::string filter, std::string sValue);
		string			GetString(std::string query, ...);
		void			DeleteEntry(std::string query, ...);
		void			DeleteEntry(std::string table, std::string filter, std::string sValue);
		void			DeleteEntry(std::string table, std::string filter, int nValue);
		CppSQLite3Query GetQuery(std::string table, std::string filter, int sValue);
		CppSQLite3Query GetQuery(std::string table, std::string filter, std::string sValue);
		CppSQLite3Query GetQuery(std::string query, ...);


};


#endif /* SQLITEDBASE_H_ */
