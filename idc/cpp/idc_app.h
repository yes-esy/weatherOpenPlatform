/**
 * @FilePath     : /dataOpenPlatform/idc/cpp/idc_app.h
 * @Description  : 此程序是共享平台项目公用函数和类的声明文件。
 * @Author       : shengYang 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : shengYang 2900226123@qq.com
 * @LastEditTime : 2025-10-17 21:46:22
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#ifndef IDC_APP_H
#define IDC_APP_H
#include "_public.h"
#include "_ooci.h"
using namespace idc;
class CZHOBTMIND
{
private:
    struct surfData_t
    {
        char siteId[6];    // 站点代码
        char datetime[21]; // 数据时间: 格式yyyymmddhh24miss,精确到分钟，秒固定天00.
        char t[11];        // 气温: 单位，0.1摄氏度
        char p[11];        // 气压: 单位，0.1百帕
        char u[11];        // 相对湿度:0-100之间的值
        char wd[11];       // 风向:0~360之间的值
        char wf[11];       // 风速:单位0.1m/s
        char r[11];        // 降雨量:0.1mm
        char vis[11];      // 能见度
    };
    clogfile &mLogFile;         // 日志文件
    connection &mDatabaseCon;   // 数据库连接引用
    sqlstatement mStatement;    // 数据库操作
    string mLineBuffer;         // 从文件中读取的一行
    surfData_t mSurfRecordBind; // 气象观测数据结构体变量
public:
    CZHOBTMIND(connection &inDatabaseCon, clogfile &inLogFile) : mLogFile(inLogFile), mDatabaseCon(inDatabaseCon)
    {
    }
    ~CZHOBTMIND() {}
    /**
     * @brief        : 把文件总读到的一行数据拆分到surfData_t结构体中;
     * @param         {string &} stringLine: 读取到的一行
     * @return        {bool} :返回true
     **/
    bool splitBuffer(const string &stringLine, const bool isXml);
    /**
     * @brief        : 把surData插入到ZHOBTMIND表中
     * @return        {bool} :返回true
     **/
    bool insertTable();
};

#endif