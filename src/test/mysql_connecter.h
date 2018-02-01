
#pragma once


#include "mysql_connection.h"
#include <string>
#include <map>
#include "typedef.h"


class DbConnector
{
public:
	DbConnector()
		:m_try_recon_cnt(0)
	{}
	bool Init(const std::string &hostName, const std::string &userName, const std::string &password, const std::string &db_name);

	bool ExecuteInsertSql(const std::string &sql);

private:
	bool connect_db_server();

private:
	sql::Connection* m_con;
	uint32_t m_try_recon_cnt;
	std::string m_db_name;
	std::string m_hostName;
	std::string m_userName;
	std::string m_password;

};

int TestMysql();

