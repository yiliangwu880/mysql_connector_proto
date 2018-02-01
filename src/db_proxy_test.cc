#include "db_proxy.h"
#include "proto/types.pb.h"
#include "config.h"

using namespace std;
using namespace db_proto;
using namespace google::protobuf;

namespace
{

	class MyHandleDbResult : public HandleDbResult
	{
	public:
		virtual void SelectResult(db_proto::RspSelectTable &rsp) override
		{
			unique_ptr<google::protobuf::Message> msg = DbProxy::CreateMessage(rsp.msg_name());
			if (nullptr == msg)
			{
				LOG_ERROR("selectData create message fail. name=%s", rsp.msg_name().c_str());
				return;
			}
			if (!msg->ParseFromString(rsp.data()))
			{
				LOG_ERROR("selectData error, parse fail")
					return;
			}
			LOG_DEBUG("select result=\n%s", msg->DebugString().c_str());
			LOG_DEBUG("is last=%d", rsp.is_last());
		};

	private:
	};

	const char *s_mysql_ip = "192.168.141.196";
	const char *s_mysql_psw = "123456";
	const char *s_mysql_user = "root";
	const char *s_mysql_db = "test";

	DbProxy &GetProxy()
	{
		static MyHandleDbResult handler;
		static DbProxy proxy(s_mysql_ip, s_mysql_user, s_mysql_psw, s_mysql_db, handler);
		return proxy;
	}
	void t1()
	{
		ReqCreateTable msg;
		msg.set_msg_name("db_proto.TestTable");
		GetProxy().TryCreateTable(msg);

	}
	void updateData()
	{
		TestTable t;
		t.set_id(2);
		t.set_name_32(32);
		t.set_name_bool(false);
		t.set_name_enum(db_proto::T2);
		ReqUpdateData msg;
		msg.set_data(t.SerializeAsString());
		if (!GetProxy().Update(msg))
		{
			LOG_DEBUG("update fail");
		}
		else
		{
			LOG_DEBUG("update ok");
		}
	}

	void insertData()
	{
		TestTable t;
		t.set_id(1);
		ReqInsertData msg;
		msg.set_msg_name(t.GetDescriptor()->full_name());

		{
			msg.set_data(t.SerializeAsString());
			if (!GetProxy().Insert(msg))
			{
				LOG_DEBUG("insert fail");
			}
			else
			{
				LOG_DEBUG("insert ok");
			}
		}

		{
			t.set_id(2);
			t.set_name_bool(true);
			t.set_name_enum(db_proto::T1);
			string s;
			s.append(1, '\0');
			s += "a";
			s.append(1, '\0');
			t.set_name_bytes(s);
			t.set_name_str("ab");

			msg.set_data(t.SerializeAsString());
			if (!GetProxy().Insert(msg))
			{
				LOG_DEBUG("insert fail");
			}
			else
			{
				LOG_DEBUG("insert ok");
			}

			//设置id3,值一样
			t.set_id(3);
			msg.set_data(t.SerializeAsString());
			if (!GetProxy().Insert(msg))
			{
				LOG_DEBUG("insert fail");
			}
			else
			{
				LOG_DEBUG("insert ok");
			}
		}
	}
	void executeSql()
	{
		string str = "UPDATE TestTable SET name_32=222 WHERE id=2";
		if (GetProxy().ExecuteSql(str))
		{
			LOG_DEBUG("-------------test update ok--------------");
		}
	}
	void selectData()
	{
		TestTable t;
		ReqSelectTable req;
		req.set_msg_name(t.GetDescriptor()->full_name());
		req.set_cond("id=2");
		//req.set_limit_num(1);

		GetProxy().Select(req);


	}
	void DelData()
	{
		TestTable t;
		ReqDelData req;
		req.set_msg_name(t.GetDescriptor()->full_name());
		req.set_cond("id=3");


		if (GetProxy().Del(req))
		{
			LOG_DEBUG("-------------test del ok--------------");
		}


	}
}



void db_proxy_test()
{
	t1();
	insertData();
	updateData();
	executeSql();
	selectData();
	DelData();
	while (true)
	{
		char c = getchar();
		switch (c)
		{
		default:
			break;
		case 'a':
			selectData();
			break;
		case 'e':
			LOG_DEBUG("===========end=============");
			return;
		}
	}

}



