#pragma once

#include "mysql_connection.h"
#include <string>
#include <map>
#include "typedef.h"
#include "proto/types.pb.h"
#include <google/protobuf/descriptor.h>
#include <memory>
#include "cppconn/resultset.h"

const int MAX_SQL_STR_SIZE = 1024 * 1024 * 4;

class HandleDbResult
{
public:
	virtual void SelectResult(db_proto::RspSelectTable &res) = 0;

private:
};

class DbProxy
{
public:
	DbProxy(const std::string ip, const std::string user, const std::string psw, const std::string db_name, HandleDbResult &handler);
	~DbProxy();

	static std::unique_ptr<google::protobuf::Message> CreateMessage(const std::string &msg_name);

	void TryCreateTable(const db_proto::ReqCreateTable &req); //如果表存在，测不会再创建
	bool Insert(const db_proto::ReqInsertData &req);
	bool Update(const db_proto::ReqUpdateData &req);
	bool Select(const db_proto::ReqSelectTable &req);
	bool Del(const db_proto::ReqDelData &req);
	bool ExecuteSql(const std::string &sql_str);

private:
	bool connect_db();
	bool CreateSql(const db_proto::ReqCreateTable &req, std::string &sql_str);
	std::string GetCreateTypeStr(google::protobuf::FieldDescriptor::Type t);

	bool CreateInsertSql(const google::protobuf::Message &msg, std::string &sql_str);
	bool CreateUpdateSql(const google::protobuf::Message &msg, std::string &sql_str);
	//使用动态分配message对象。 消除内存管理烦恼，自动释放。
	//失败返回 nullptr
	void SetInsertPreparePara(sql::PreparedStatement &pstmt, google::protobuf::Message &msg);
	void SetUpdatePreparePara(sql::PreparedStatement &pstmt, google::protobuf::Message &msg);
	bool SetField(google::protobuf::Message &msg, const google::protobuf::FieldDescriptor &field, const sql::ResultSet &res);
private:
	std::string m_ip;
	std::string m_user;
	std::string m_psw;
	std::string m_db_name;
	sql::Connection* m_con = nullptr;
	HandleDbResult &m_handle_result;
};

