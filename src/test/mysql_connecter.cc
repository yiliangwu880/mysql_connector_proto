#include "mysql_connecter.h"
#include <cppconn/driver.h>
#include <mysql_driver.h>
#include <stdio.h>
#include <stddef.h>
#include <cppconn/exception.h>
#include <cppconn/statement.h>
#include "./config.h"
#include "proto/types.pb.h"

using namespace std;
using namespace db_proto;

static const char *s_mysql_ip = "192.168.141.196";
static const char *s_mysql_psw = "123456";
static const char *s_mysql_user = "root";
static const char *s_mysql_db = "test";

bool DbConnector::Init(const std::string &hostName, const std::string &userName, const std::string &password, const std::string &db_name)
{
	m_hostName = hostName;
	m_userName = userName;
	m_password = password;
	m_db_name = db_name;
	return connect_db_server();
}

bool DbConnector::connect_db_server()
{
	sql::Driver* driver = NULL;
	try {
		driver = sql::mysql::get_driver_instance();
	}
	catch (sql::SQLException &e) {
		APP_ERROR_LOG("%s, MySQL error code:%d, SQLState:%s", e.what(), e.getErrorCode(), e.getSQLStateCStr());
		return false;
	}

	sql::ConnectOptionsMap connection_properties;
	connection_properties["hostName"] = m_hostName;
	connection_properties["userName"] = m_userName;
	connection_properties["password"] = m_password;
	//connection_properties["OPT_RECONNECT"] = true;

	LOG_DEBUG("try connect mysql db[%s]. this may need a few minute!", m_hostName.c_str());
	try {
		m_con = driver->connect(connection_properties);
	}
	catch (sql::SQLException &e) {
		APP_ERROR_LOG("%s, MySQL error code:%d", e.what(), e.getErrorCode());
		m_con = NULL;
		return true; //连接失败，照样先跑着
	}
	catch (...) {
		LOG_DEBUG("connect mysql fail %p", m_con);
		m_con = NULL;
		return true;
	}

	if (NULL == m_con)
	{
		APP_ERROR_LOG("connect fail");
		return false;
	}
	m_con->setSchema(m_db_name);

	LOG_DEBUG("connect mysql db ok");
	return true;
}



bool DbConnector::ExecuteInsertSql(const std::string &sql)
{
	if (NULL == m_con)
	{
		APP_ERROR_LOG("obj not initialize");
#ifdef DEBUG
		if (0 == m_try_recon_cnt++ % 600) //初始化没连接好服务器，600分钟尝试一次
#else
		if (0 == m_try_recon_cnt++ % 10) //初始化没连接好服务器，10分钟尝试一次
#endif
		{
			connect_db_server();
		}
		return false;
	}
	if (sql.length()>1000*3)
	{
		APP_ERROR_LOG("sql string too long");
		return false;
	}


	sql::Statement* statement = m_con->createStatement();
	if (NULL == statement)
	{
		APP_ERROR_LOG("unknow error");
		return false;
	}
	auto_ptr<sql::Statement> auto_del(statement);

	try {
		statement->execute(sql);
		int affect_row = statement->getUpdateCount();
		if (affect_row > 0)
		{
			//APP_DEBUG_LOG("affect rows, %d", affect_row);
			return true;
		}
		return false;
	}
	catch (sql::SQLException &e) {
		APP_ERROR_LOG("%s, MySQL error code:%d, SQLState:%s", e.what(), e.getErrorCode(), e.getSQLStateCStr());

		//不行，检测是断线,每60次（60分钟）就尝试重连
		if (!m_con->isValid())
		{
			if (0 == m_try_recon_cnt++ % 60)
			{
				if ( m_con->reconnect())
				{
					m_con->setSchema(m_db_name);
					m_try_recon_cnt = 0; 
					KEY_LOG("try re connect ok");
				}
				else
				{
					KEY_LOG("try re connect fail");
				}
			}
		}
		return false;
	}
}
namespace{

	void Test1()
	{
		DbConnector d;
		string hostName = s_mysql_ip, userName = s_mysql_user, password = s_mysql_psw, db_name = s_mysql_db;
		d.Init(hostName, userName, password, db_name);
		//string s = "INSERT INTO `tb_test` VALUES ('123', '604210');"; 
		string s =
			//"SET FOREIGN_KEY_CHECKS=0;"
		"DROP TABLE IF EXISTS `tb_test2`;";
		d.ExecuteInsertSql(s);

		s=	"CREATE TABLE `tb_test2` ("
			"`a` varchar(11) NOT NULL,"
			"PRIMARY KEY(`a`)"
			") ENGINE = InnoDB DEFAULT CHARSET = utf8;";


		d.ExecuteInsertSql(s);
	}
	void Test2()
	{
		sql::Driver* driver = sql::mysql::get_driver_instance();
		LOG_DEBUG("driveraddr=%ld", driver); driver = sql::mysql::get_driver_instance();
		LOG_DEBUG("driveraddr=%ld", driver);
	}
	void test3()
	{
		TTT3  d;
		d.set_t(32);
		d.set_t2(33);
		string s = d.SerializeAsString();
		d.Clear();
		
		for (auto &v : s)
		{
			LOG_DEBUG("%d", v);
		}
	}

}//end namespace{
int TestMysql()
{
	Test1();
	Test2();
	test3();
	return 0;
}