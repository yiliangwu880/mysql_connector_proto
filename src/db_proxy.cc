#include "db_proxy.h"
#include <cppconn/driver.h>
#include <cppconn/datatype.h>
#include <mysql_driver.h>
#include "config.h"
#include <cppconn/prepared_statement.h>
#include <google/protobuf/message.h>

using namespace std;
using namespace google::protobuf;
using namespace db_proto;

DbProxy::DbProxy(const std::string ip, const std::string user, const std::string psw, const std::string db_name, HandleDbResult &handler)
:m_ip(ip)
, m_user(user)
, m_psw(psw)
, m_db_name(db_name)
, m_handle_result(handler)
{
	connect_db();
}

DbProxy::~DbProxy()
{
	if (nullptr != m_con)
	{
		delete m_con;
	}
}

void DbProxy::TryCreateTable(const db_proto::ReqCreateTable &req)
{
	try {
		if (nullptr == m_con)
		{
			LOG_ERROR("nullptr == m_con");
			return;
		}
		std::auto_ptr< sql::PreparedStatement > pstmt;

		string sql_str;
		if (!CreateSql(req, sql_str))
		{
			LOG_ERROR("CreateSql fail");
			return;
		}

		//sql 模板
		//"CREATE TABLE `tb_test2` ("
		//	"`a` varchar(11) NOT NULL,"
		//	"PRIMARY KEY(`a`)"
		//	") ENGINE = InnoDB DEFAULT CHARSET = utf8;"
		//LOG_DEBUG("create sql=%s", sql_str.c_str());
		pstmt.reset(m_con->prepareStatement(sql_str));
		int affect_row = pstmt->executeUpdate();
		if (0 != affect_row)
		{
			LOG_ERROR("create table fail. row=%d, table name[%s]", affect_row, req.msg_name().c_str());
			return;
		}
	}
	catch (sql::SQLException &e) {
		APP_ERROR_LOG("%s, MySQL error code:%d, SQLState:%s", e.what(), e.getErrorCode(), e.getSQLStateCStr());
		return;
	}
}

bool DbProxy::Insert(const db_proto::ReqInsertData &req)
{
	if (nullptr == m_con)
	{
		LOG_ERROR("nullptr == m_con");
		return false;
	}
	unique_ptr<google::protobuf::Message> msg = CreateMessage(req.msg_name());
	if (nullptr == msg)
	{
		LOG_ERROR("create message fail. name=%s", req.msg_name().c_str());
		return false;
	}
	msg->ParseFromString(req.data()); 
	//LOG_DEBUG("msg content=%s", msg->DebugString().c_str());
	try {
		unique_ptr< sql::PreparedStatement > pstmt;
		string sql_str;
		CreateInsertSql(*msg, sql_str);
	
		//LOG_DEBUG("create insert sql=%s", sql_str.c_str());
		pstmt.reset(m_con->prepareStatement(sql_str));
		SetInsertPreparePara(*pstmt, *msg);
		int affect_row = pstmt->executeUpdate();
		if (1 != affect_row)
		{
			LOG_ERROR("insert table fail. row=%d, table name[%s]", affect_row, req.msg_name().c_str());
			return false;
		}
		return true;
	}
	catch (sql::SQLException &e) {
		APP_ERROR_LOG("%s, MySQL error code:%d, SQLState:%s", e.what(), e.getErrorCode(), e.getSQLStateCStr());
		return false;
	}
}

bool DbProxy::Update(const db_proto::ReqUpdateData &req)
{
	if (nullptr == m_con)
	{
		LOG_ERROR("nullptr == m_con");
		return false;
	}
	unique_ptr<google::protobuf::Message> msg = CreateMessage(req.msg_name());
	if (nullptr == msg)
	{
		LOG_ERROR("create message fail. name=%s", req.msg_name().c_str());
		return false;
	}
	msg->ParseFromString(req.data());
	try {
		string sql_str;
		if (!CreateUpdateSql(*msg, sql_str))
		{
			LOG_ERROR("create update sql fail");
			return false;
		}
		//LOG_DEBUG("create update sql=%s", sql_str.c_str());
		unique_ptr< sql::PreparedStatement > pstmt(m_con->prepareStatement(sql_str));
		SetUpdatePreparePara(*pstmt, *msg);
		int affect_row = pstmt->executeUpdate();
		if (1 != affect_row)
		{
			LOG_ERROR("update table fail. row=%d, table name[%s]", affect_row, req.msg_name().c_str());
			return false;
		}
		return true;
	}
	catch (sql::SQLException &e) {
		APP_ERROR_LOG("%s, MySQL error code:%d, SQLState:%s", e.what(), e.getErrorCode(), e.getSQLStateCStr());
		return false;
	}
}

bool DbProxy::SetField(Message& msg, const FieldDescriptor &field, const sql::ResultSet& res)
{
	const Reflection* rfl  = msg.GetReflection();
	if (nullptr == rfl)
		return false;

	if (field.is_repeated())
	{
		APP_ERROR_LOG("not support repeated lable");
		return false;
	}
	if (res.isNull(field.name()))
	{
		return true;
	}
	try {
		switch (field.type())
		{
		default:
			APP_ERROR_LOG("unknow type %d", (int)field.type());
			return false;
		case FieldDescriptor::TYPE_DOUBLE:
				rfl->SetDouble(&msg, &field, (double)res.getDouble(field.name()));
				break;
		case FieldDescriptor::TYPE_FLOAT:
			rfl->SetFloat(&msg, &field, (float)res.getDouble(field.name()));
				break;
		case FieldDescriptor::TYPE_INT64:
		case FieldDescriptor::TYPE_SINT64:
		case FieldDescriptor::TYPE_SFIXED64:
			rfl->SetInt64(&msg, &field, res.getInt64(field.name()));
				break;
		case FieldDescriptor::TYPE_UINT64:
		case FieldDescriptor::TYPE_FIXED64:
			rfl->SetUInt64(&msg, &field, res.getUInt64(field.name()));
				break;
		case FieldDescriptor::TYPE_FIXED32:
		case FieldDescriptor::TYPE_UINT32:
			rfl->SetUInt32(&msg, &field, res.getUInt(field.name()));
				break;
		case FieldDescriptor::TYPE_INT32:
		case FieldDescriptor::TYPE_SFIXED32:
		case FieldDescriptor::TYPE_SINT32:
			rfl->SetInt32(&msg, &field, res.getInt(field.name()));
			break;
		case FieldDescriptor::TYPE_ENUM:
			{
				const EnumDescriptor *pEnumDes = field.enum_type();
				const EnumValueDescriptor* pV = pEnumDes->FindValueByNumber( res.getInt( field.name() ) );
				if (nullptr == pV)
				{
					LOG_ERROR("can't find Enum value. value=%d", res.getInt(field.name()) );
					break;
				}
				rfl->SetEnum(&msg, &field, pV);
			}
			break;
		case FieldDescriptor::TYPE_BOOL:
			rfl->SetBool(&msg, &field, res.getBoolean(field.name()));
				break;
		case FieldDescriptor::TYPE_STRING:
			rfl->SetString(&msg, &field, res.getString(field.name()));
				break;
		case FieldDescriptor::TYPE_BYTES:
			rfl->SetString(&msg, &field, res.getString(field.name()));
			break;
		case FieldDescriptor::TYPE_MESSAGE:
			{
				unique_ptr<std::istream> inStream( res.getBlob(field.name()) );
				if (nullptr == inStream)
				{
					APP_ERROR_LOG("res.getBlob() fail field.name()=%s", field.name().c_str());
					return false;
				}
				//NULL不解析消息
				if (res.wasNull())
				{
					return true;
				}
				Message* chiledMsg = rfl->MutableMessage(&msg, &field);
				if (nullptr == chiledMsg)
				{
					APP_ERROR_LOG("MutableMessage fail field.name()=%s", field.name().c_str());
					return false;
				}

				if (!chiledMsg->ParseFromIstream(inStream.get()))
				{
					LOG_ERROR("解析失败！");
					return false;
				}
				break;
			}
		}//end switch (field.type())
	} catch (sql::InvalidArgumentException e) {
		//结果集中没有改字段，不进行设置
		APP_ERROR_LOG("can not set filed %s, ", field.name().c_str());
		APP_ERROR_LOG("%s, MySQL error code:%d, SQLState:%d", e.what(), e.getErrorCode(), e.getSQLStateCStr());
		return false;
	} catch (sql::SQLException &e) {
		APP_ERROR_LOG("%s, MySQL error code:%d, SQLState:%s", e.what(), e.getErrorCode(), e.getSQLStateCStr());
		return false;
	}
	return true;
}

namespace
{
	const char * NumToStr(int num)
	{
		static char buf[100];
		snprintf(buf, sizeof(buf), "%d", num);
		return buf;
	}
	void CreateSelectSql(const db_proto::ReqSelectTable &req, const string &table_name, string &sql_str)
	{
		sql_str = "SELECT ";
		sql_str += "* FROM ";
		sql_str += table_name;
		if (!req.cond().empty())
		{
			sql_str += " WHERE " + req.cond();
		}
		if (req.has_limit_num())
		{
			sql_str += " limit ";
			sql_str += NumToStr(req.limit_num());
		}
	}
}
bool DbProxy::Select(const db_proto::ReqSelectTable &req)
{
	if (nullptr == m_con)
	{
		LOG_ERROR("nullptr == m_con");
		return false;
	}

	unique_ptr<google::protobuf::Message> msg = CreateMessage(req.msg_name());
	if (nullptr == msg)
	{
		LOG_ERROR("create message fail. name=%s", req.msg_name().c_str());
		return false;
	}
	const google::protobuf::Descriptor* des = msg->GetDescriptor();
	if (nullptr == des)
	{
		APP_ERROR_LOG("error msg name [%s]", req.msg_name().c_str());
		return false;
	}

	try {
		string sql_str;
		CreateSelectSql(req, des->name(), sql_str);

		LOG_ERROR("select sql [%s]", sql_str.c_str());
		unique_ptr< sql::Statement > stmt(m_con->createStatement());
		stmt->execute(sql_str);

		int row_num = 0;
		unique_ptr<sql::ResultSet> ret;
		do 
		{
			ret.reset(stmt->getResultSet());
			if (0 == row_num && nullptr == ret) //一个数据都没有
			{
				LOG_ERROR("execute sql fail [%s]", sql_str.c_str());
				return false;
			}
			while (ret->next()) {
				msg->Clear();
				int count = des->field_count();
				for (int i = 0; i < count; i++)
				{
					const FieldDescriptor* field = des->field(i);
					if (!SetField(*msg, *field, *ret))
					{
						LOG_ERROR("set field failed, filed_name[%s]", field->name().c_str());
						return false;
					}
				}
				db_proto::RspSelectTable rsp;
				rsp.set_msg_name(req.msg_name());
				rsp.set_data(msg->SerializeAsString());
				if (ret->isLast())
				{
					rsp.set_is_last(true);
				}
				else
				{
					rsp.set_is_last(false);
				}
				row_num++;
				m_handle_result.SelectResult(rsp);
			}
		} while (stmt->getMoreResults());
		return true;
	}
	catch (sql::SQLException &e) {
		APP_ERROR_LOG("%s, MySQL error code:%d, SQLState:%s", e.what(), e.getErrorCode(), e.getSQLStateCStr());
		return false;
	}
}

bool DbProxy::Del(const db_proto::ReqDelData &req)
{
	if (nullptr == m_con)
	{
		LOG_ERROR("nullptr == m_con");
		return false;
	}
	if (req.cond().empty())
	{
		LOG_ERROR("req.cond().empty()");
		return false;
	}
	const google::protobuf::Descriptor* des =
		google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(req.msg_name());
	if (nullptr == des)
	{
		return false;
	}
	try {
		string sql_str = "delete from ";
		sql_str += des->name();
		sql_str += " where ";
		sql_str += req.cond();

		unique_ptr< sql::Statement > stmt(m_con->createStatement());
		stmt->execute(sql_str);
		int affect_row = stmt->getUpdateCount();
		if (affect_row == 0)
		{
			LOG_ERROR("del fail, affect rows, %d", affect_row);
			return false;
		}
		return true;
	}
	catch (sql::SQLException &e) {
		APP_ERROR_LOG("%s, MySQL error code:%d, SQLState:%s", e.what(), e.getErrorCode(), e.getSQLStateCStr());
		return false;
	}
}

bool DbProxy::ExecuteSql(const std::string &sql_str)
{
	if (nullptr == m_con)
	{
		LOG_ERROR("nullptr == m_con");
		return false;
	}
	try {
		unique_ptr< sql::Statement > stmt(m_con->createStatement());
		stmt->execute(sql_str);
		//int affect_row = stmt->getUpdateCount(); //这个判断执行有没有作用不行
		//if (affect_row == 0)
		//{
		//	LOG_ERROR("execute sql fail %d [%s]", r, sql_str.c_str());
		//	return false;
		//}
		return true;
	}
	catch (sql::SQLException &e) {
		APP_ERROR_LOG("%s, MySQL error code:%d, SQLState:%s", e.what(), e.getErrorCode(), e.getSQLStateCStr());
		return false;
	}
}

bool DbProxy::connect_db()
{
	sql::Driver* driver = NULL;
	try {
		driver = sql::mysql::get_driver_instance();

		sql::ConnectOptionsMap connection_properties;
		connection_properties["hostName"] = m_ip;
		connection_properties["userName"] = m_user;
		connection_properties["password"] = m_psw;
		connection_properties["OPT_RECONNECT"] = true;

		LOG_DEBUG("try connect mysql db[%s]. this may need a few minute!", m_user.c_str());
		m_con = driver->connect(connection_properties);
		m_con->setSchema(m_db_name);
		LOG_DEBUG("connect mysql db ok");
		return true;
	}
	catch (sql::SQLException &e) {
		APP_ERROR_LOG("%s, MySQL error code:%d, SQLState:%s", e.what(), e.getErrorCode(), e.getSQLStateCStr());
		return false;
	}

}

bool DbProxy::CreateSql(const db_proto::ReqCreateTable &req, std::string &sql_str)
{
	const google::protobuf::Descriptor* des =
		google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(req.msg_name());
	if (nullptr == des)
	{
		LOG_ERROR("find msg Descriptor fail. msg name=%s", req.msg_name().c_str());
		return false;
	}
	if (0 == des->field_count())
	{
		LOG_ERROR("msg no field. msg name=%s", req.msg_name().c_str());
		return false;
	}

	sql_str = "CREATE TABLE IF NOT EXISTS `";
	sql_str += des->name(); //不用 req.msg_name() 去掉命名空间, 因为表名有.符号会导致insert sql语句查表名失败
	sql_str += "` (";

	int cnt = des->field_count();
	bool is_find_key = false;
	for (int i = 0; i < cnt; i++)
	{
		//每个域 字符串模板 = "`a` varchar(11) NOT NULL,"
		const FieldDescriptor* field = des->field(i);
		if (nullptr == field)
		{
			LOG_ERROR("unknow error %d", i);
			return false;
		}	
		if (field->is_repeated())
		{
			LOG_ERROR("not support repeated lable");
			return false;
		}
		sql_str += "`";
		sql_str += field->name();
		sql_str += "` ";	
		if ("id" == field->name())
		{
			is_find_key = true;
			sql_str += "bigint(20) NOT NULL";
		}
		else
		{
			sql_str += GetCreateTypeStr(field->type());
		}
		sql_str += ",";
		//LOG_DEBUG("check field[%s]", field->name().c_str());
	}
	if (!is_find_key)
	{
		LOG_ERROR("msg need field name [id] for key. msg name=%s", req.msg_name().c_str());
		return false;
	}
	sql_str += "PRIMARY KEY(`id`)";
	sql_str += ") ENGINE = InnoDB DEFAULT CHARSET = utf8;";
	return true;
}


std::string DbProxy::GetCreateTypeStr(FieldDescriptor::Type t)
{
	switch (t)
	{
	default:
		LOG_ERROR("unknow type %d", (int)t);
		return "unknow type";
	case FieldDescriptor::TYPE_DOUBLE:
	case FieldDescriptor::TYPE_FLOAT:
		return "double NULL DEFAULT NULL";
	case FieldDescriptor::TYPE_INT64:
	case FieldDescriptor::TYPE_SINT64:
	case FieldDescriptor::TYPE_SFIXED64:
	case FieldDescriptor::TYPE_UINT64:
	case FieldDescriptor::TYPE_FIXED64:
	case FieldDescriptor::TYPE_FIXED32:
	case FieldDescriptor::TYPE_UINT32:
		return "bigint(20) NULL DEFAULT NULL";
	case FieldDescriptor::TYPE_INT32:
	case FieldDescriptor::TYPE_ENUM:
	case FieldDescriptor::TYPE_SFIXED32:
	case FieldDescriptor::TYPE_SINT32:
	case FieldDescriptor::TYPE_BOOL:
		return "int(11) NULL DEFAULT NULL";
	case FieldDescriptor::TYPE_STRING:
		return "varchar(255) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL";
	case FieldDescriptor::TYPE_MESSAGE:
	case FieldDescriptor::TYPE_BYTES:
		return "blob NULL";
	}
}

bool DbProxy::CreateInsertSql(const google::protobuf::Message &msg, std::string &sql_str)
{
	const Descriptor* des = msg.GetDescriptor();
	//const Reflection* rfl = msg.GetReflection();
	//if (nullptr == rfl)
	//{
	//	LOG_ERROR("find msg Descriptor or Reflection fail. msg name=%s", des->name().c_str());
	//	return false;
	//}
	if (nullptr == des )
	{
		LOG_ERROR("find msg Descriptor or Reflection fail. msg name=%s", des->name().c_str());
		return false;
	}
	int count = des->field_count();
	if (0 == count)
	{
		LOG_ERROR("msg no field. msg name=%s", des->name().c_str());
		return false;
	}

	sql_str += "INSERT INTO ";
	sql_str += des->name();
	sql_str += "(";

	//循环拷贝名字
	for (int i = 0; i < count; i++)
	{
		const FieldDescriptor* field = des->field(i);
		//if (!rfl->HasField(msg, field))
		//{
		//	continue;
		//}
		if (i>0)
		{
			sql_str += ",";
		}
		sql_str += field->name();
	}

	sql_str += ") VALUES(";

	for (int i = 0; i < count; i++)
	{
		if (i>0)
		{
			sql_str += ",?";
		}
		else
		{
			sql_str += "?";
		}
	}

	sql_str += ");";
	return true;
}


bool DbProxy::CreateUpdateSql(const google::protobuf::Message &msg, std::string &sql_str)
{
	const Descriptor* des = msg.GetDescriptor();
	const Reflection* rfl = msg.GetReflection();
	if (nullptr == rfl)
	{
		LOG_ERROR("find msg Descriptor or Reflection fail. msg name=%s", des->name().c_str());
		return false;
	}
	if (nullptr == des)
	{
		LOG_ERROR("find msg Descriptor or Reflection fail. msg name=%s", des->name().c_str());
		return false;
	}
	int count = des->field_count();
	if (0 == count)
	{
		LOG_ERROR("msg no field. msg name=%s", des->name().c_str());
		return false;
	}

	sql_str += "UPDATE ";
	sql_str += des->name();
	sql_str += " SET ";

	unsigned long long id = 0;
	bool need_comma = false;
	for (int i = 0; i < count; i++)
	{
		const FieldDescriptor* field = des->field(i);
		if (!rfl->HasField(msg, field))
		{
			continue;
		}
		if ("id" == field->name())
		{
			id = rfl->GetUInt64(msg, field);
			continue;
		}
		if (need_comma)
		{
			sql_str += ",";
		}
		sql_str += field->name();
		sql_str += "=?";
		need_comma = true;
	}
	if (0 == id)
	{
		APP_ERROR_LOG("find id fail");
		return false;
	}
	sql_str += " ";
	char where[100];
	snprintf(where, sizeof(where), "WHERE id=%llu  limit 1", id);
	sql_str += where;

	sql_str += ";";
	return true;
}

unique_ptr<google::protobuf::Message> DbProxy::CreateMessage(const std::string &msg_name)
{
	unique_ptr<google::protobuf::Message> p;
	const google::protobuf::Descriptor* des =
		google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(msg_name);
	if (nullptr == des)
	{
		return p;
	}
	const google::protobuf::Message* prototype =
		google::protobuf::MessageFactory::generated_factory()->GetPrototype(des);
	if (nullptr == prototype)
	{
		return p;
	}
	p.reset(prototype->New());
	return p;
}

namespace{

	class DataBuf : public std::streambuf
	{
	public:
		DataBuf() {}

		DataBuf(char * d, size_t s) {
			setg(d, d, d + s);
		}

		void InitBuf(char * d, size_t s) {
			setg(d, d, d + s);
		}
	};

	void SetFieldParam(google::protobuf::Message &msg, const FieldDescriptor &field, sql::PreparedStatement &pstmt, int idx)
	{
		const Reflection* rfl = msg.GetReflection();
		if ( !rfl)
		{
			LOG_ERROR("error para");
			return;
		}

		if (field.is_repeated())
		{
			LOG_ERROR("not support repeated lable");
			return;
		}
		if (!rfl->HasField(msg, &field))
		{
			pstmt.setNull(idx, sql::DataType::INTEGER); //参数2 无用的，随便填
			return;
		}
		
		switch (field.type())
		{
		default:
			LOG_ERROR("unknow type %d", (int)field.type());
			break;
		case FieldDescriptor::TYPE_DOUBLE:
			pstmt.setDouble(idx, rfl->GetDouble(msg, &field));
			break;
		case FieldDescriptor::TYPE_FLOAT:
			pstmt.setDouble(idx, rfl->GetFloat(msg, &field));
			break;
		case FieldDescriptor::TYPE_INT64:
		case FieldDescriptor::TYPE_SINT64:
		case FieldDescriptor::TYPE_SFIXED64:
			pstmt.setInt64(idx, rfl->GetInt64(msg, &field));
			break;
		case FieldDescriptor::TYPE_UINT64:
		case FieldDescriptor::TYPE_FIXED64:
			pstmt.setUInt64(idx, rfl->GetUInt64(msg, &field));
			break;
		case FieldDescriptor::TYPE_FIXED32:
		case FieldDescriptor::TYPE_UINT32:
			pstmt.setUInt(idx, rfl->GetUInt32(msg, &field));
			break;
		case FieldDescriptor::TYPE_INT32:
		case FieldDescriptor::TYPE_SFIXED32:
		case FieldDescriptor::TYPE_SINT32:
			pstmt.setInt(idx, rfl->GetInt32(msg, &field));
			break;
		case FieldDescriptor::TYPE_ENUM:
		{
			const EnumValueDescriptor* ev = rfl->GetEnum(msg, &field);
			if (nullptr == ev)
			{
				LOG_ERROR("nullptr == ev");
				break;
			}
			pstmt.setInt(idx, ev->number());
			break;
		}
		case FieldDescriptor::TYPE_BOOL:
			pstmt.setBoolean(idx, rfl->GetBool(msg, &field));
			break;
		case FieldDescriptor::TYPE_STRING:
			//if (!rfl->HasField(msg, &field)) //留着，无值情况运行没问题就删除
			//{
			//	pstmt.setString(idx, "");
			//}
			//else
		{
			pstmt.setString(idx, rfl->GetString(msg, &field));
		}

			break;
		case FieldDescriptor::TYPE_BYTES:
			// LOG_ERROR("这个存档bytes还有问题（没测试完整），先加上可以避免整个存档失败。");
			if (!rfl->HasField(msg, &field))
			{
				pstmt.setString(idx, "");
			}
			else
			{
				pstmt.setString(idx, rfl->GetString(msg, &field));
			}
			break;
		case FieldDescriptor::TYPE_MESSAGE:
			{
				if (!rfl->HasField(msg, &field))
				{
					break;
				}

				Message* chiledMsg = rfl->MutableMessage(&msg, &field);
				char str[MAX_SQL_STR_SIZE];
				bool r = chiledMsg->SerializeToArray(str, sizeof(str));
				if (!r)
				{
					LOG_ERROR("SerializeToArray fail");
					break;
				}

				DataBuf buffer;
				std::istream blobStream(&buffer);
				buffer.InitBuf(str, chiledMsg->ByteSize());
				blobStream.rdbuf(&buffer);
				pstmt.setBlob(idx, &blobStream);
				break;
			}
		} //end switch (field.type())
	}

} //namespace{

void DbProxy::SetInsertPreparePara(sql::PreparedStatement &pstmt, google::protobuf::Message &msg)
{
	const Descriptor* des = msg.GetDescriptor();
	if (nullptr == des)
	{
		LOG_ERROR("null == descriptor")
		return;
	}

	int cnt = des->field_count();
	if (cnt < 1)
	{
		LOG_ERROR( "no field");
		return;
	}

	for (int i = 0; i < cnt; i++)
	{
		const FieldDescriptor* field = des->field(i);
		SetFieldParam(msg, *field, pstmt, i + 1);
	}

	return;
}
void DbProxy::SetUpdatePreparePara(sql::PreparedStatement &pstmt, google::protobuf::Message &msg)
{
	const Descriptor* des = msg.GetDescriptor();
	if (nullptr == des)
	{
		LOG_ERROR("null == descriptor")
		return;
	}
	const Reflection* rfl = msg.GetReflection();
	if (nullptr == rfl)
	{
		LOG_ERROR("find msg Descriptor or Reflection fail. msg name=%s", des->name().c_str());
		return;
	}

	int cnt = des->field_count();
	if (cnt < 1)
	{
		LOG_ERROR("no field");
		return;
	}

	int idx = 1;
	for (int i = 0; i < cnt; i++)
	{
		const FieldDescriptor* field = des->field(i);
		if (!rfl->HasField(msg, field))
		{
			continue;
		}
		if ("id" == field->name())
		{
			continue;
		}
		SetFieldParam(msg, *field, pstmt, idx);
		idx++;
	}

	return;
}
