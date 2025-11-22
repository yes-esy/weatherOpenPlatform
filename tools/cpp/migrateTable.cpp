/**
 * @FilePath     : /project/dataOpenPlatform/tools/cpp/migrateTable.cpp
 * @Description  :
 * @Author       : shengYang 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : shengYang 2900226123@qq.com
 * @LastEditTime : 2025-10-22 10:41:00
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/

#include "_tools.h"
using namespace idc;

struct runArg_t
{
    char connStr[101];          // 数据库连接参数
    char srcTableName[31];      // 源表名
    char destTableName[31];     // 目的表名
    char keyCol[31];            // 带迁移的表的唯一键字段名
    char whereCondition[10001]; // 满足的条件
    int maxCount;               // 执行的记录数
    char startTime[31];         // 运行时间的区间
    char procName[51];          // 进程名
    int timeout;                // 超时时间
} runArg;

// 显示程序的帮助
void _help();

// 把xml解析到参数runArg结构中
bool _xmlToRunArg(const char *strXMLBuffer);

clogfile logFile;

// 判断当前时间是否在程序运行的时间区间内。
bool instarttime();

connection oracleConn;

// 业务处理主函数。
bool _migrateTable();

void EXIT(int sig);

cpactive procAct;
bool _migrateTable();

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        _help();
        return -1;
    }

    // 关闭全部的信号和输入输出
    // 处理程序退出的信号
    closeioandsignal();
    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);

    if (logFile.open(argv[1]) == false)
    {
        printf("open log file failed, log file :%s.\n", argv[1]);
        return -1;
    }

    // 把xml解析到参数runArg结构中
    if (_xmlToRunArg(argv[2]) == false)
        return -1;

    // 判断当前时间是否在程序运行的时间区间内。
    if (instarttime() == false)
    {
        // return 0;
    }
    procAct.addpinfo(runArg.timeout, runArg.procName);
    if (oracleConn.connecttodb(runArg.connStr, "Simplified Chinese_China.AL32UTF8", true) != 0) // 字符集随便填，开启自动提交。
    {
        logFile.write("connect database(%s) failed.\n%s\n", runArg.connStr, oracleConn.message());
        EXIT(-1);
    }
    // 业务处理主函数。
    _migrateTable();
}
/**
 * @brief        : 显示程序的帮助
 * @return        {void}
 **/
void _help()
{
    printf("Using:/project/dataOpenPlatform/tools/bin/migrateTable logfilename xmlbuffer\n\n");

    printf("Sample:/project/dataOpenPlatform/tools/bin/procctl 3600 /project/dataOpenPlatform/tools/bin/migrateTable /log/idc/migrateTable_ZHOBTMIND1.log "
           "\"<connStr>idc/idcpwd</connStr>"
           "<srcTableName>T_ZHOBTMIND1</srcTableName>"
           "<destTableName>T_ZHOBTMIND1_HIS</destTableName>"
           "<keyCol>rowid</keyCol>"
           "<where>where visit_date<sysdate-0.03</where>"
           "<maxCount>10</maxCount>"
           "<startTime>22,23,00,01,02,03,04,05,06,13</startTime>"
           "<timeout>120</timeout>"
           "<procName>migrateTable_ZHOBTMIND1</procName>\"\n\n");

    printf("本程序是共享平台的公共功能模块，用于迁移表中的数据。\n");

    printf("logfilename 本程序运行的日志文件。\n");
    printf("xmlbuffer   本程序运行的参数，用xml表示，具体如下：\n\n");

    printf("connStr     数据库的连接参数，格式：username/passwd。\n");
    printf("srcTableName       待迁移数据表的源表名。\n");
    printf("srcTableName       待迁移数据表的目的表名。\n");
    printf("keyCol      待迁移数据表的唯一键字段名，可以用记录编号，如keyid，建议用rowid，效率最高。\n");
    printf("where       待迁移的数据需要满足的条件，即SQL语句中的where部分。\n");
    printf("maxCount    执行一次SQL语句删除的记录数，建议在100-500之间。\n");
    printf("startTime   程序运行的时间区间，例如02,13表示：如果程序运行时，踏中02时和13时则运行，其它时间不运行。"
           "startTime"
           "为了减少对数据库的压力，数据迁移一般在业务最闲的时候时进行。\n");
    printf("timeout     本程序的超时时间，单位：秒，建议设置120以上。\n");
    printf("procName       进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n");
}
/**
 * @brief        : 把xml解析到参数starg结构中
 * @param         {char} *strXMLBuffer: xml文件缓冲区
 * @return        {bool} : 成功——true,失败——false
 **/
bool _xmlToRunArg(const char *strXMLBuffer)
{
    memset(&runArg, 0, sizeof(runArg_t));
    getxmlbuffer(strXMLBuffer, "connStr", runArg.connStr);
    if (strlen(runArg.connStr) == 0)
    {
        logFile.write("connection string is null.\n");
        return false;
    }
    getxmlbuffer(strXMLBuffer, "srcTableName", runArg.srcTableName, 30);
    if (strlen(runArg.srcTableName) == 0)
    {
        logFile.write("srcTableName is null.\n");
        return false;
    }
    getxmlbuffer(strXMLBuffer, "destTableName", runArg.destTableName, 30);
    if (strlen(runArg.destTableName) == 0)
    {
        logFile.write("destTableName is null.\n");
        return false;
    }

    getxmlbuffer(strXMLBuffer, "keyCol", runArg.keyCol, 30);
    if (strlen(runArg.keyCol) == 0)
    {
        logFile.write("keyCol is null.\n");
        return false;
    }

    getxmlbuffer(strXMLBuffer, "where", runArg.whereCondition, 1000);
    if (strlen(runArg.whereCondition) == 0)
    {
        logFile.write("whereCondition is null.\n");
        return false;
    }

    getxmlbuffer(strXMLBuffer, "startTime", runArg.startTime, 30);

    getxmlbuffer(strXMLBuffer, "maxCount", runArg.maxCount);
    if (runArg.maxCount == 0)
    {
        logFile.write("maxCount is null.\n");
        return false;
    }

    getxmlbuffer(strXMLBuffer, "timeout", runArg.timeout);
    if (runArg.timeout == 0)
    {
        logFile.write("timeout is null.\n");
        return false;
    }

    getxmlbuffer(strXMLBuffer, "procName", runArg.procName, 50);
    if (strlen(runArg.procName) == 0)
    {
        logFile.write("procName is null.\n");
        return false;
    }

    return true;
}
/**
 * @brief        : 处理信号2和信号15
 * @param         {int} sig: 信号标志
 * @return        {void}
 **/
void EXIT(int sig)
{
    logFile.write("program exit,sig=%d\n\n", sig);

    oracleConn.disconnect();

    exit(0);
}
/**
 * @brief        : 业务处理主函数。
 * @return        {bool} : 成功——true,失败——false
 **/
bool _migrateTable()
{
    ctimer timer;
    char tmpValue[21]; // 存放待删除记录的唯一建的值

    sqlstatement selectStatement(&oracleConn);

    selectStatement.prepare("select %s from %s %s", runArg.keyCol, runArg.srcTableName, runArg.whereCondition);
    selectStatement.bindout(1, tmpValue, 20);
    // 1. 准备SQL语句
    string deleteSQLStr = sformat("delete from %s where %s in (", runArg.srcTableName, runArg.keyCol);
    for (int i = 0; i < runArg.maxCount; i++)
    {
        deleteSQLStr = deleteSQLStr + sformat(":%lu,", i + 1);
    }
    deleterchr(deleteSQLStr, ','); // 删除最后一个逗号
    deleteSQLStr += ")";
    char keyValues[runArg.maxCount][21]; // 存放唯一键字段的值的数组
    sqlstatement deleteStatement(&oracleConn);
    deleteStatement.prepare(deleteSQLStr);

    for (int i = 0; i < runArg.maxCount; i++)
    {
        deleteStatement.bindin(i + 1, keyValues[i], 20);
    }
    ObtainColumnsInfo obtainColumns;
    obtainColumns.getAllColumnsInfo(oracleConn, runArg.srcTableName);
    string selectInfoSQLStr = sformat(
        "insert into %s select %s from %s where %s in (",
        runArg.destTableName, obtainColumns.allColumnsName.c_str(),
        runArg.srcTableName, runArg.keyCol);
    for (int i = 0; i < runArg.maxCount; i++)
    {
        selectInfoSQLStr += sformat(":%lu,", i + 1);
    }
    deleterchr(selectInfoSQLStr, ',');
    selectInfoSQLStr += ")";
    sqlstatement insertStatement(&oracleConn);
    insertStatement.prepare(selectInfoSQLStr);
    for(int i = 0; i < runArg.maxCount; i ++)
    {
        insertStatement.bindin(i+1,keyValues[i],20);
    }

    if (selectStatement.execute() != 0)
    {
        logFile.write("select SQL: %s executed failed, error message: %s", selectStatement.sql(), selectStatement.message());
    }
    int count = 0;
    memset(keyValues, 0, sizeof(keyValues));
    while (true)
    {
        memset(tmpValue, 0, sizeof(tmpValue));
        if (selectStatement.next() != 0)
            break;
        strcpy(keyValues[count], tmpValue);
        count++;

        if (count == runArg.maxCount)
        {
            if(insertStatement.execute()!=0)
            {
                logFile.write("insert SQL: %s executed failed,error message: %s\n", insertStatement.sql(), insertStatement.message());
                return false;
            }
            if (deleteStatement.execute() != 0)
            {
                logFile.write("delete SQL: %s executed failed,error message: %s\n", deleteStatement.sql(), deleteStatement.message());
                return false;
            }

            oracleConn.commit();
            count = 0;
            memset(keyValues, 0, sizeof(keyValues));
            procAct.uptatime();
            break;
        }
    }
    if (count > 0)
    {
        if (insertStatement.execute() != 0)
        {
            logFile.write("insert SQL: %s executed failed,error message: %s\n", insertStatement.sql(), insertStatement.message());
            return false;
        }
        if (deleteStatement.execute() != 0)
        {
            logFile.write("delete SQL: %s executed failed,error message: %s\n", deleteStatement.sql(), deleteStatement.message());
            return false;
        }
    }
    if (selectStatement.rpc() > 0)
    {
        logFile.write("migrate  from %s to %s,%d rows in %.2f sec.\n", runArg.srcTableName, runArg.destTableName,selectStatement.rpc(), timer.elapsed());
    }
    return true;
}
/**
 * @brief        : 判断当前时间是否在程序运行的时间区间内。
 * @return        {bool} : 成功——true,失败——false
 **/
bool instarttime()
{
    if (strlen(runArg.startTime) != 0)
    {
        if (strstr(runArg.startTime, ltime1("hh24").c_str()) == 0)
            return false;
    }
    return true;
}
