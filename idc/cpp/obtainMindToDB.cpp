/**
 * @FilePath     : /dataOpenPlatform/idc/cpp/obtainMindToDB.cpp
 * @Description  : 气象观测数据入库模块
 * @Author       : shengYang 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : shengYang 2900226123@qq.com
 * @LastEditTime : 2025-10-18 13:35:12
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "_public.h"
#include "_ooci.h"
using namespace idc;
void EXIT(int sig);   // 退出处理函数
cpactive procActive;  // 进程心跳
clogfile logFile;     // 日志文件
connection oracleCon; // 数据库连接引用

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

bool _obtainMindToDB(const char *pathname, const char *connstr, const char *charset); // 业务处理主函数
int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        printf("\n");
        printf("Using:./obtainMindToDB inifile connstr charset logfile\n");

        printf("Example:/project/dataOpenPlatform/tools/bin/procctl 10 /project/dataOpenPlatform/idc/bin/obtainMindToDB /idcdata/surfdata "
               "\"idc/idcpwd\" \"Simplified Chinese_China.AL32UTF8\" /log/idc/obtmindtodb.log\n\n");
        printf("本程序用于把全国气象观测参数数据保存到数据库的T_ZHOBTMIND表中，支持xml和csv两种格式，数据只插入不更新。\n");
        printf("inifile 全国气象站点参数文件名（全路径）。\n");
        printf("connstr 数据库连接参数：username/password@tnsname\n");
        printf("charset 数据库的字符集。\n");
        printf("logfile 本程序运行的日志文件名。\n");
        printf("程序每120秒运行一次，由procctl调度。\n\n\n");
        return -1;
    }
    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);

    // 打开日志文件。
    if (logFile.open(argv[4]) == false)
    {
        printf("打开日志文件失败（%s）。\n", argv[4]);
        return -1;
    }
    procActive.addpinfo(10, "obtainCodeToDB");

    // 连接数据库
    if (oracleCon.connecttodb(argv[2], argv[3]) != 0)
    {
        logFile.write("connect database(%s) failed.\n%s\n", argv[2], oracleCon.message());
        EXIT(-1);
    }
    logFile.write("connect database(%s) succeed.\n", argv[2]);

    // 准备插入和更新表的sql语句
    // 业务处理主函数。
    _obtainMindToDB(argv[1], argv[2], argv[3]);
    return 0;
}
/**
 * @brief        : 业务处理主函数
 * @param         {char} *pathname: 存放气象观测数据文件的目录
 * @param         {char} *connstr: 连接数据库的参数数据库账号和密码
 * @param         {char} *charset: 数据库字符集
 * @return        {*}
 **/
bool _obtainMindToDB(const char *pathname, const char *connstr, const char *charset)
{
    // 1) 打开存放气象观测数据文件的目录
    cdir dir;
    if (dir.opendir(pathname, "*.xml,*.csv") == false)
    {
        logFile.write("dir.open(%s) failed.\n", pathname);
        return false;
    }
    CZHOBTMIND ZHOBTMIND(oracleCon, logFile); // 操作观测数据表对象
    // 2) 用循环读取目录中的每个文件
    while (true)
    {
        // 读取一个气象数据观测文件,只处理*.xml和*.csv
        if (dir.readdir() == false)
        {
            break;
        }
        // logFile.write("current file is %s\n",dir.m_ffilename.c_str());

        // 如果有文件需要处理,判断数据库连接状态,如果未连接,连接数据库。
        if (oracleCon.isopen() == false)
        {
            if (oracleCon.connecttodb(connstr, charset) != 0) // 连接失败
            {
                logFile.write("connect database failed. connect string is %s, Error message : %s \n", connstr, oracleCon.message());
            }
        }
        logFile.write("connect database(%s) succeed.\n", connstr);

        // 打开文件读取文件的每一行,插入到数据库的表中
        cifile inFile;
        if (inFile.open(dir.m_ffilename) == false) // 打开失败
        {
            logFile.write("file(%s) open failed,method :inFile.open(%s).\n", dir.m_ffilename.c_str(), dir.m_ffilename);
            return false;
        }
        int totalRecords = 0;                            // 文件总记录数
        int insertRecords = 0;                           // 成功插入的记录数
        ctimer timer;                                    // 计时器
        string stringBuffer;                             // 存放从文件中读取的一行数据
        bool isXml = matchstr(dir.m_ffilename, "*.xml"); // 文件格式:true xml,false csv
        if (!isXml)                                      // csv文件去掉表头
        {
            inFile.readline(stringBuffer);
        }
        while (true)
        {
            if (isXml == true)
            {

                if (inFile.readline(stringBuffer, "<endl/>") == false) // xml文件结束的格式
                {
                    break;
                }
            }
            else
            {
                if (inFile.readline(stringBuffer, "<endl/>") == false) // csv文件没有结束标志
                {
                    break;
                }
            }
            totalRecords++;
            ZHOBTMIND.splitBuffer(stringBuffer, isXml);
            // 将数据插入表中
            if (ZHOBTMIND.insertTable() == true)
            {
                insertRecords++;
            }
        }
        logFile.write("proceed file(%s), total records = %d , insert records = %d, time = %.2f s\n", dir.m_ffilename.c_str(), totalRecords, insertRecords, timer.elapsed());
        inFile.closeandremove();
        oracleCon.commit();
    }
    return true;
}

/**
 * @brief        :
 * @param         {int} sig:
 * @return        {*}
 **/
void EXIT(int sig)
{
    logFile.write("程序退出，sig=%d\n\n", sig);

    // 可以不写，在析构函数中会回滚事务和断开与数据库的连接。
    oracleCon.rollback();
    oracleCon.disconnect();

    exit(0);
}
