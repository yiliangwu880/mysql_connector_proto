
#pragma once

#include "./utility/logFile.h"
// windows testPrj ������
class DebugLog;
extern DebugLog g_logFile;



#define DEBUG_MODE

   //ERROR ��vs2012�Ļ���h�ļ� �ض��� �ĳ�LOG_ERROR�� linux������û�ģ�ע��

#define LOG_INFO(x, ...)  g_logFile.printf(LOG_LV_INFO, __FILE__, __LINE__, __FUNCTION__, x, ##__VA_ARGS__);
#define LOG_DEBUG(x, ...)  g_logFile.printf(LOG_LV_DEBUG, __FILE__, __LINE__, __FUNCTION__, x, ##__VA_ARGS__);
#define LOG_ERROR(x, ...)  g_logFile.printf(LOG_LV_ERROR, __FILE__, __LINE__, __FUNCTION__, x, ##__VA_ARGS__);
#define LOG_WARN(x, ...)  g_logFile.printf(LOG_LV_WARN, __FILE__, __LINE__, __FUNCTION__, x, ##__VA_ARGS__);
#define LOG_FATAL(x, ...)  g_logFile.printf(LOG_LV_FATAL, __FILE__, __LINE__, __FUNCTION__, x, ##__VA_ARGS__);


#define APP_ERROR_LOG LOG_ERROR
#define APP_DEBUG_LOG LOG_DEBUG
#define KEY_LOG LOG_INFO

#define LOG_CONDIT(cond, ret, x, ...)\
	do{\
	if(!(cond)){\
	g_logFile.printf(LOG_LV_ERROR, __FILE__, __LINE__, __FUNCTION__, x, ##__VA_ARGS__);\
	return ret;\
	}	\
	}while(0)

#define LOG_CONDIT_VOID(cond, x, ...)\
	do{\
	if(!(cond))	\
	{\
		g_logFile.printf(LOG_LV_ERROR, __FILE__, __LINE__, __FUNCTION__, x, ##__VA_ARGS__);\
		return; \
	}\
	}while(0)

#define CONDIT(cond, ret)\
	do{\
	if(!(cond)){\
	return ret;\
	}	\
	}while(0)

#define CONDIT_VOID(cond)\
	do{\
	if(!(cond))	\
	return; \
	}while(0)

#define ASSERT_DEBUG(cond)\
	do{\
	if(!(cond)){\
	g_logFile.printf(LOG_LV_ERROR, __FILE__, __LINE__, __FUNCTION__, "assert");\
	cout << __FILE__ << ":" <<  __LINE__ << "assert error" << endl;\
	}	\
	}while(0)



