#pragma once
#include "typedef.h"
#include <string>



enum DebugLogLv
{
    //���ȼ��Ӹߵ���
    LOG_LV_FATAL,
    LOG_LV_ERROR,
    LOG_LV_WARN,
    LOG_LV_DEBUG,
    //����ļ��𣬲�����ļ�λ����Ϣ
    LOG_LV_INFO,
    LOG_LV_ANY
};

class DebugLog 
{
public:
    //para:const char *fname, �ļ�·����
    //para:const char *prefix_nameÿ����־ǰ׺�ַ���,ͨ�����ļ�����д
	explicit DebugLog(const char *fname, const char *prefix_name = "");
    ~DebugLog();
	void printf(DebugLogLv lv, const char * file, int line, const char *pFun, const char * pattern, ...);
    void setShowLv(DebugLogLv lv);
    //print log in std out.
    void setStdOut(bool is_std_out);
    void flush();
private:
    const char *GetLogLevelStr( DebugLogLv lv ) const;

    void print(const char * pattern, ...);
private:
    DebugLogLv m_log_lv;
    FILE *m_file;
    bool m_is_std_out;
    std::string m_prefix_name;
};
