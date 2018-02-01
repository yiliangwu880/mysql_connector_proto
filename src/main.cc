#include "stdio.h"
#include "config.h"
#include "./test/mysql_connecter.h"

DebugLog g_logFile("log.txt");

void db_proxy_test();

int main(int argc, char* argv[]) 
{
 
	LOG_DEBUG("=========================start=========================");
	//TestMysql();
	db_proxy_test();
	return 0;
}

