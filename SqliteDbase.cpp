/*
 * SqliteDbase.cpp
 *
 *  Created on: Mar 23, 2012
 *      Author: Gnarls
 */

#include "SqliteDbase.h"

SqliteDbase * SqliteDbase::Get()
{
	static SqliteDbase ref;
	return &ref;
}

bool SqliteDbase::Open()
{
	try
	{
		if ( !db )
		{
			db = new CppSQLite3DB();
			db->open("data/micromud.db");
		}
	}
	catch (CppSQLite3Exception& e)
	{
		std::cout << "[SqliteDbase] Failed to open [" << e.errorCode() << ":" << e.errorMessage() << "\n\r";
	}


	if ( !db )
		return false;

	return true;
}

void SqliteDbase::Close()
{
	db->close();
	delete db;
	db = NULL;
}

CppSQLite3Table SqliteDbase::GetTable( CppSQLite3Buffer bufSQL )
{
	CppSQLite3Table t;

	if ( !db && !Open() )
		return t;

	try
	{
		t = db->getTable(bufSQL);
		return t;
	}
	catch (CppSQLite3Exception& e)
	{
		std::cout << "[SqliteDbase] Unable to get table [" << e.errorCode() << ":" << e.errorMessage() << "]\n\r";
		throw e;
	}

	return t;
}

bool SqliteDbase::ExecExists(const char * szSQL)
{
	if ( !db && !Open() )
		return false;
	try
	{
		return db->recordExists(szSQL);
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "[SqliteDbase] - recordExists failed " << e.errorCode() << ":" << e.errorMessage() << "\n\r";
	}

	return false;
}

int SqliteDbase::ExecDML(const char* szSQL)
{
	if ( !db && !Open() )
		return -1;
	try
	{
		return db->execDML(szSQL);
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "[SqliteDbase] - ExecScalar failed " << e.errorCode() << ":" << e.errorMessage() << "\n\r";
	}

	return 0;
}

int SqliteDbase::ExecScalar(const char* szSQL)
{
	if ( !db && !Open() )
		return -1;
	try
	{
		return db->execScalar(szSQL);
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "[SqliteDbase] - ExecScalar failed " << e.errorCode() << ":" << e.errorMessage() << "\n\r";
	}
	return 0;
}

string SqliteDbase::ExecString (const char* szSQL )
{
	string output;

	if ( !db && !Open() )
		return "";

	try
	{
		CppSQLite3Query q = db->execQuery(szSQL);
		if ( q.eof() )
		{
			q.finalize();
			return "";
		}
		if (q.fieldIsNull(0))
			return "";

		output = q.fieldValue(0);
		q.finalize();

		return output;
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "[SqliteDbase] - ExecString failed " << e.errorCode() << ":" << e.errorMessage() << "\n\r";
	}

	return "";
}

CppSQLite3Query SqliteDbase::ExecQuery(const char* szSQL)
{
	CppSQLite3Query query;

	try
	{
		query = db->execQuery(szSQL);
		return query;
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "[SqliteDbase] - ExecString failed " << e.errorCode() << ":" << e.errorMessage() << "\n\r";
	}

	return query;
}

int	SqliteDbase::GetInt(std::string table, std::string field, std::string filter, int nValue)
{
	stringstream sSQL;
	sSQL << "select " << field << " from " << table << " where " << filter << "=" << nValue << ";";
	return SqliteDbase::Get()->ExecScalar(sSQL.str().c_str());
}

int	SqliteDbase::GetInt(std::string table, std::string field, std::string filter, std::string sValue)
{
	stringstream sSQL;
	sSQL << "select " << field << " from " << table << " where " << filter << " like '" << sValue << "';";
	return SqliteDbase::Get()->ExecScalar(sSQL.str().c_str());
}

int SqliteDbase::GetInt(std::string query, ...)
{
	char buf[MAX_STRING_LENGTH*2];
	va_list args;

	va_start(args, query);
	vsprintf(buf, query.c_str(), args);
	va_end(args);

	string sEdit(buf);

	return SqliteDbase::Get()->ExecScalar(sEdit.c_str());
}

bool SqliteDbase::RecordExists(std::string table, std::string field, std::string filter, int nValue)
{
	stringstream sSQL;
	sSQL << "select " << field << " from " << table << " where " << filter << "=" << nValue << ";";
	return SqliteDbase::Get()->ExecExists(sSQL.str().c_str());
}

bool SqliteDbase::RecordExists(std::string table, std::string field, std::string filter, std::string sValue)
{
	stringstream sSQL;
	sSQL << "select " << field << " from " << table << " where " << filter << " like '" << sValue << "';";
	return SqliteDbase::Get()->ExecExists(sSQL.str().c_str());
}

string SqliteDbase::GetString(std::string table, std::string field, std::string filter, int nValue)
{
	stringstream sSQL;
	sSQL << "select " << field << " from " << table << " where " << filter << "=" << nValue << ";";
	return SqliteDbase::Get()->ExecString(sSQL.str().c_str());
}

string SqliteDbase::GetString(std::string table, std::string field, std::string filter, std::string sValue)
{
	stringstream sSQL;
	sSQL << "select " << field << " from " << table << " where " << filter << " like '" << sValue << "';";
	return SqliteDbase::Get()->ExecString(sSQL.str().c_str());
}

string SqliteDbase::GetString(std::string query, ...)
{
	char buf[MAX_STRING_LENGTH*2];
	va_list args;

	va_start(args, query);
	vsprintf(buf, query.c_str(), args);
	va_end(args);

	string sEdit(buf);

	return SqliteDbase::Get()->ExecString(sEdit.c_str());
}

void SqliteDbase::DeleteEntry(std::string query, ...)
{
	char buf[MAX_STRING_LENGTH*2];
	va_list args;

	va_start(args, query);
	vsprintf(buf, query.c_str(), args);
	va_end(args);

	string sEdit(buf);

	SqliteDbase::Get()->ExecDML(sEdit.c_str());
	return;
}

void SqliteDbase::DeleteEntry(std::string table, std::string filter, std::string sValue)
{
	stringstream sSQL;
	sSQL << "delete from " << table << " where " << filter << " like '" << sValue << "';";
	SqliteDbase::Get()->ExecDML(sSQL.str().c_str());
	return;
}

void SqliteDbase::DeleteEntry(std::string table, std::string filter, int nValue)
{
	stringstream sSQL;
	sSQL << "delete from " << table << " where " << filter << "=" << nValue << ";";
	SqliteDbase::Get()->ExecDML(sSQL.str().c_str());
	return;
}

CppSQLite3Query SqliteDbase::GetQuery(std::string table, std::string filter, int nValue)
{
	stringstream sSQL;
	sSQL << "select * from " << table << " where " << filter << "=" << nValue << ";";
	return SqliteDbase::Get()->ExecQuery(sSQL.str().c_str());
}


CppSQLite3Query SqliteDbase::GetQuery(std::string table, std::string filter, std::string sValue)
{
	stringstream sSQL;
	sSQL << "select * from " << table << " where " << filter << " like '" << sValue << "';";
	return SqliteDbase::Get()->ExecQuery(sSQL.str().c_str());
}

CppSQLite3Query SqliteDbase::GetQuery(std::string query, ...)
{
	char buf[MAX_STRING_LENGTH*2];
	va_list args;

	va_start(args, query);
	vsprintf(buf, query.c_str(), args);
	va_end(args);

	string sEdit(buf);

	return SqliteDbase::Get()->ExecQuery(sEdit.c_str());
}

