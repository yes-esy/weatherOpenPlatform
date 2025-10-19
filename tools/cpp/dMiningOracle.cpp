/**
 * @FilePath     : /project/dataOpenPlatform/tools/cpp/dMiningOracle.cpp
 * @Description  : 用于从Oracle数据库源表抽取数据，生成xml文件。
 * @Author       : shengYang 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : shengYang 2900226123@qq.com
 * @LastEditTime : 2025-10-18 21:46:30
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "_public.h"
#include "_ooci.h"
using namespace idc;
// 程序运行参数的结构体。
struct arg_t
{

    char connectStr[101];   // 数据库的连接参数。
    char charset[51];       // 数据库的字符集。
    char selectSQL[1024];   // 从数据源数据库抽取数据的SQL语句。
    char fieldStr[501];     // 抽取数据的SQL语句输出结果集字段名，字段名之间用逗号分隔。
    char fieldLen[501];     // 抽取数据的SQL语句输出结果集字段的长度，用逗号分隔。
    char eFileName[31];     // 输出xml文件的后缀。
    char bFileName[31];     // 输出xml文件的前缀。
    char outpath[256];      // 输出xml文件存放的目录。
    int maxCount;           // 输出xml文件最大记录数，0表示无限制。xml文件将用于入库，如果文件太大，数据库会产生大事务。
    char startTime[52];     // 程序运行的时间区间
    char incrField[31];     // 递增字段名。
    char incrFileName[256]; // 已抽取数据的递增字段最大值存放的文件。
    char connectStr1[101];  // 已抽取数据的递增字段最大值存放的数据库的连接参数。
    int timeout;            // 进程心跳的超时时间。
    char procName[51];      // 进程名，建议用"dminingoracle_后缀"的方式。
} arg;
ccmdstr fieldName;                    // 结果集字段名数组。
ccmdstr fieldLen;                     // 结果集字段长度数组。
clogfile logFile;                     // 日志对象
connection oracleCon;                 // 数据库连接对象
long maxIncrValue;                    // 递增字段最大值
int incrFieldPos = -1;                // 增量字段在结果集中的位置
cpactive procActive;                     // 进程心跳。
void EXIT(int sig);                   // 程序退出和信号2、15的处理函数。
void _help();                         // 帮助文档
bool xmlLoadArg(const char *xmlFile); // 解析xml文件到结构体
bool inStartTime();                   // 判断当前时间是否在程序运行的时间区域内
bool dMingOracle();                   // 业务主函数
bool readIncrMaxValue();              // 从数据库或者arg.incrFileName文件中读取maxIncrValue
bool writeIncrField();                // 将已抽取的数据的最大值写入数据库表或者arg.incrFileName文件
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        _help();
        return -1;
    }

    // 关闭全部的信号和输入输出。
    // 设置信号,在shell状态下可用 "kill + 进程号" 正常终止些进程。
    // 但请不要用 "kill -9 +进程号" 强行终止。
    // closeioandsignal(true);
    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);
    // 打开日志
    if (logFile.open(argv[1]) == false)
    {
        printf("打开日志文件失败（%s）。\n", argv[1]);
        return -1;
    }
    // 解析xml文件到结构体arg
    if (xmlLoadArg(argv[2]) == false)
    {
        logFile.write("xml file load args failed, xml file :%s.\n", argv[2]);
        EXIT(-1);
    }
    // 判断当前时间是否在程序运行的时间区域内
    if (inStartTime() == false)
    {
        return 0;
    }
    procActive.addpinfo(arg.timeout, arg.procName); // 把进程的心跳信息写入共享内存。

    // 连接数据源的数据库
    if (oracleCon.connecttodb(arg.connectStr, arg.charset) != 0)
    {
        logFile.write("connect database(%s) failed.\n%s\n", arg.connectStr, oracleCon.message());
        EXIT(-1);
    }
    logFile.write("connect database(%s) succeed.\n", arg.connectStr);
    if (readIncrMaxValue() == false) // 读取失败程序退出
    {
        EXIT(-1);
    }
    dMingOracle(); // 业务函数

    return 0;
}
/**
 * @brief        : 将已抽取的数据的最大值写入数据库表或者arg.incrFileName文件
 * @return        {bool} : 写入成功返回true,失败返回false
 **/
bool writeIncrField()
{
    // 非增量抽取
    if (strlen(arg.incrField) == 0)
    {
        return true;
    }

    if (strlen(arg.connectStr1) != 0) // 建立数据库连接,写入数据库
    {
        connection conn;
        if (conn.connecttodb(arg.connectStr1, arg.charset) != 0) // 连接失败
        {
            logFile.write("connect database failed, connect string :%s,error message:%s\n", arg.connectStr1, conn.message());
            return false;
        }
        sqlstatement stmt(&conn);
        stmt.prepare("update T_MAXINCVALUE set max_incr_value = :1 where proc_name=:2");
        stmt.bindin(1, maxIncrValue);
        stmt.bindin(2, arg.procName);
        if (stmt.execute() != 0)
        {
            if (stmt.rc() == 942) // 如果表不存在返回942
            {
                // 创建表
                stmt.execute("create table T_MAXINCVALUE(proc_name varchar2(250),max_incr_value number(15),primary key(proc_name))");
                stmt.execute("insert into T_MAXINCVALUE values('%s',%ld)", arg.procName, maxIncrValue);
                conn.commit();
                return true;
            }
            else
            {
                logFile.write("SQL:%s execute failed, error message:%s\n", stmt.sql(), stmt.message());
                return false;
            }
        }
        else
        {
            if (stmt.rpc() == 0)
            {
                stmt.execute("insert into T_MAXINCVALUE values('%s',%ld)", arg.procName, maxIncrValue);
            }
            conn.commit();
        }
    }
    else // 写入文件
    {
        // 把递增字段的最大值写入文件。
        cofile outFile;

        if (outFile.open(arg.incrFileName, false) == false)
        {
            logFile.write("file(%s) open failed,method:outFile.open(arg.incrFileName, false).\n", arg.incrFileName);
            return false;
        }

        // 把已抽取数据的最大id写入文件。
        outFile.writeline("%ld", maxIncrValue);
    }

    return true;
}
/**
 * @brief        : 从数据库或者arg.incrFileName文件中读取maxIncrValue
 * @return        {bool} : 读取成功返回true,失败返回false
 **/
bool readIncrMaxValue()
{
    maxIncrValue = 0;               // 初始化为0
    if (strlen(arg.incrField) == 0) // arg.incrField为空不是增量抽取
    {
        return true;
    }
    // 查找递增字段在结果集中的位置
    for (int i = 0; i < fieldName.size(); i++)
    {
        if (fieldName[i] == arg.incrField)
        {
            incrFieldPos = i;
            break;
        }
    }
    if (incrFieldPos == -1)
    {
        logFile.write("increment field %s inexist list %s.\n", arg.incrField, arg.fieldStr);
        return false;
    }
    // 从数据库表加载递增字段最大值
    if (strlen(arg.connectStr1) != 0) //
    {
        connection ownConn;
        if (ownConn.connecttodb(arg.connectStr1, arg.charset) != 0) // 连接失败
        {
            logFile.write("connect database failed, connection string : %s, Error message:%s\n", arg.connectStr1, ownConn.message());
            return false;
        }
        sqlstatement stmt(&ownConn);
        stmt.prepare("select max_incr_value from T_MAXINCVALUE where proc_name=:1");
        stmt.bindin(1, arg.procName);
        stmt.bindout(1, maxIncrValue);
        if (stmt.execute() != 0) // 执行失败
        {
            logFile.write("SQL:%s execute failed,error message:%s", stmt.sql(), stmt.message());
        }
        stmt.next();
    }
    else // 从文件中加载递增字段最大值
    {
        cifile inFile;
        if (inFile.open(arg.incrFileName) == false) // 打开失败
        {
            return true;
        }
        string temp;
        inFile.readline(temp);
        maxIncrValue = stoi(temp);
    }
    logFile.write("last extract data position: %s = %ld .\n", arg.incrField, maxIncrValue);
    return true;
}
/**
 * @brief        : 业务主函数
 * @return        {bool} : 返回true
 **/
bool dMingOracle()
{
    // 1. 准备抽取数据的SQL语句
    sqlstatement statement(&oracleCon);
    statement.prepare(arg.selectSQL);
    // 2. 绑定结果集变量
    string strFieldValue[fieldName.size()];
    for (int i = 1; i <= fieldName.size(); i++)
    {
        statement.bindout(i, strFieldValue[i - 1], stoi(fieldLen[i - 1]));
    }
    // 如果是增量抽取，绑定输入参数（已抽取数据的递增字段的最大值）。
    if (strlen(arg.incrField) != 0)
        statement.bindin(1, maxIncrValue);
    procActive.uptatime();
    // 3. 执行抽取数据的SQL语句
    if (statement.execute() != 0) // 执行失败写日志
    {
        logFile.write("SQL: %s execute failed,Error message : %s.\n", statement.sql(), statement.message());
        return false;
    }
    // 4. 打开xml文件
    string xmlFileName;
    cofile outFile;
    int xmlSeq = 1; // xml文件序号

    // 5. 获取结果集中的记录,写入xml文件
    while (true)
    {
        if (statement.next() != 0)
        {
            break;
        }
        if (outFile.isopen() == false) // 结果集中有数据,打开文件
        {
            sformat(xmlFileName, "%s/%s_%s_%s_%d.xml",
                    arg.outpath, arg.bFileName, ltime1("yyyymmddhh24miss").c_str(),
                    arg.eFileName, xmlSeq++);
            // 写入文件
            if (outFile.open(xmlFileName) == false)
            {
                logFile.write("file(%s) open failed,metod: outFile.open().\n", xmlFileName);
                return false;
            }
            outFile.writeline("<data>\n"); // 写入数据集
        }
        // 写入字段
        for (int i = 1; i <= fieldName.size(); i++)
        {
            outFile.writeline("<%s>%s</%s>", fieldName[i - 1].c_str(), strFieldValue[i - 1].c_str(), fieldName[i - 1].c_str());
        }
        outFile.writeline("<endl/>\n"); // 写入每行结束标志

        if (arg.maxCount > 0 && (statement.rpc() % arg.maxCount == 0)) // 写满了maxCount行,关闭文件
        {
            outFile.writeline("</data>\n"); // 写入数据集结束标签
            // 关 闭文件
            if (outFile.closeandrename() == false) // 关闭失败
            {
                logFile.write("xml file(%s) close failed.\n", xmlFileName.c_str());
                return false;
            }
            logFile.write("generate file: %s effects rows: %d.\n", xmlFileName.c_str(), statement.rpc());
            procActive.uptatime();
        }
        if (strlen(arg.incrField) != 0 && maxIncrValue < stol(strFieldValue[incrFieldPos])) // 更新递增字段最大值
        {
            maxIncrValue = stol(strFieldValue[incrFieldPos]);
        }
    }
    // maxCount == 0 或者xml文件中写入的记录数不足maxCount,再此关闭文件
    if (outFile.isopen() == true) // 文件为打开才需要关闭
    {
        outFile.writeline("</data>\n"); // 写入数据集结束标签
        // 关闭文件
        if (outFile.closeandrename() == false) // 关闭失败
        {
            logFile.write("xml file(%s) close failed.\n", xmlFileName.c_str());
            return false;
        }
        if (arg.maxCount == 0)
        {
            logFile.write("generate file: %s effects rows: %d.\n", xmlFileName.c_str(), statement.rpc());
        }
        else
        {
            logFile.write("generate file: %s effects rows: %d.\n", xmlFileName.c_str(), statement.rpc() % arg.maxCount);
        }
    }
    // 将已抽取的数据的最大值写入数据库表
    if (statement.rpc() > 0)
    {
        writeIncrField();
    }

    return true;
}
/**
 * @brief        : 判断当前时间是否在程序运行的时间区域内
 * @return        {bool} : 在返回true,不在返回false
 **/
bool inStartTime()
{
    if (strlen(arg.startTime) != 0)
    {
        string strhh24 = ltime1("hh24");                 // 获取当前的小时时间
        if (strstr(arg.startTime, strhh24.c_str()) == 0) // 是否为当前小时
        {
            return false;
        }
    }
    return true;
}
/**
 * @brief        : 解析xml文件到结构体
 * @param         {char} *xmlFile: 读取到的xml文件
 * @return        {bool} : 成功返回true,失败返回false
 **/
bool xmlLoadArg(const char *xmlFile)
{
    memset(&arg, 0, sizeof(arg_t));

    getxmlbuffer(xmlFile, "connectStr", arg.connectStr, 100); // 数据源数据库的连接参数。
    if (strlen(arg.connectStr) == 0)
    {
        logFile.write("connectStr is null.\n");
        return false;
    }

    getxmlbuffer(xmlFile, "charset", arg.charset, 50); // 数据库的字符集。
    if (strlen(arg.charset) == 0)
    {
        logFile.write("charset is null.\n");
        return false;
    }

    getxmlbuffer(xmlFile, "selectSQL", arg.selectSQL, 1000); // 从数据源数据库抽取数据的SQL语句。
    if (strlen(arg.selectSQL) == 0)
    {
        logFile.write("selectSQL is null.\n");
        return false;
    }

    getxmlbuffer(xmlFile, "fieldStr", arg.fieldStr, 500); // 结果集字段名列表。
    if (strlen(arg.fieldStr) == 0)
    {
        logFile.write("fieldStr is null.\n");
        return false;
    }

    getxmlbuffer(xmlFile, "fieldLen", arg.fieldLen, 500); // 结果集字段长度列表。
    if (strlen(arg.fieldLen) == 0)
    {
        logFile.write("fieldLen is null.\n");
        return false;
    }

    getxmlbuffer(xmlFile, "bFileName", arg.bFileName, 30); // 输出xml文件前缀。
    if (strlen(arg.bFileName) == 0)
    {
        logFile.write("bFileName is null.\n");
        return false;
    }

    getxmlbuffer(xmlFile, "eFileName", arg.eFileName, 30); // 输出xml文件的后缀。
    if (strlen(arg.eFileName) == 0)
    {
        logFile.write("eFileName is null.\n");
        return false;
    }

    getxmlbuffer(xmlFile, "outpath", arg.outpath, 255); // 输出xml文件的路径。
    if (strlen(arg.outpath) == 0)
    {
        logFile.write("outpath is null.\n");
        return false;
    }

    getxmlbuffer(xmlFile, "maxCount", arg.maxCount); // 输出xml文件的最大记录数，可选参数。

    getxmlbuffer(xmlFile, "startTime", arg.startTime, 50); // 程序运行的时间区间，可选参数。

    getxmlbuffer(xmlFile, "incrField", arg.incrField, 30); // 递增字段名，可选参数。

    getxmlbuffer(xmlFile, "incrFileName", arg.incrFileName, 255); // 已抽取数据的递增字段最大值存放的文件，可选参数。

    getxmlbuffer(xmlFile, "connectStr1", arg.connectStr1, 100); // 已抽取数据的递增字段最大值存放的数据库的连接参数，可选参数。

    getxmlbuffer(xmlFile, "timeout", arg.timeout); // 进程心跳的超时时间。
    if (arg.timeout == 0)
    {
        logFile.write("timeout is null.\n");
        return false;
    }

    getxmlbuffer(xmlFile, "procName", arg.procName, 50); // 进程名。
    if (strlen(arg.procName) == 0)
    {
        logFile.write("procName is null.\n");
        return false;
    }

    // 拆分arg.fieldStr到fieldName中。
    fieldName.splittocmd(arg.fieldStr, ",");

    // 拆分arg.fieldLen到fieldLen中。
    fieldLen.splittocmd(arg.fieldLen, ",");

    // 判断fieldName和fieldLen两个数组的大小是否相同。
    if (fieldLen.size() != fieldName.size())
    {
        logFile.write("fieldStr和fieldLen的元素个数不一致。\n");
        return false;
    }

    // 如果是增量抽取，incrFileName和connectStr1必二选一。
    if (strlen(arg.incrField) > 0)
    {
        if ((strlen(arg.incrFileName) == 0) && (strlen(arg.connectStr1) == 0))
        {
            logFile.write("如果是增量抽取，incrFileName和connectStr1必二选一，不能都为空。\n");
            return false;
        }
    }

    return true;
}
/**
 * @brief        : 帮助文档
 * @return        {*}
 **/
void _help()
{
    printf("Using:/project/dataOpenPlatform/tools/bin/dMiningOracle logFileName xmlBuffer\n\n");

    printf("Sample:/project/dataOpenPlatform/tools/bin/procctl 3600 /project/dataOpenPlatform/tools/bin/dMiningOracle /log/idc/dminingoracle_ZHOBTCODE.log "
           "\"<connectStr>idc/idcpwd</connectStr><charset>Simplified Chinese_China.AL32UTF8</charset>"
           "<selectSQL>select site_id,city_name,province_name,latitude,longitude,height from T_ZHOBTCODE where site_id like '5%%%%'</selectSQL>"
           "<fieldStr>site_id,city_name,province_name,latitude,longitude,height</fieldStr><fieldLen>5,30,30,10,10,10</fieldLen>"
           "<bFileName>ZHOBTCODE</bFileName><eFileName>togxpt</eFileName><outpath>/idcdata/dmindata</outpath>"
           "<timeout>30</timeout><procName>dminingoracle_ZHOBTCODE</procName>\"\n\n");
    printf("       /project/dataOpenPlatform/tools/bin/procctl   30 /project/dataOpenPlatform/tools/bin/dMiningOracle /log/idc/dminingoracle_ZHOBTMIND.log "
           "\"<connectStr>idc/idcpwd</connectStr>"
           "<charset>Simplified Chinese_China.AL32UTF8</charset>"
           "<selectSQL>select site_id,to_char(visit_date,'yyyymmddhh24miss'),t,p,u,wd,wf,r,vis,key_id from T_ZHOBTMIND where  site_id like '5%%%%'</selectSQL>"
           "<fieldStr>site_id,visit_date,t,p,u,wd,wf,r,vis,key_id</fieldStr>"
           "<fieldLen>5,19,8,8,8,8,8,8,8,15</fieldLen>"
           "<bFileName>ZHOBTMIND</bFileName>"
           "<eFileName>togxpt</eFileName>"
           "<outpath>/idcdata/dmindata</outpath>"
           "<startTime></startTime>"
           "<incrField>key_id</incrField>"
           "<incrFileName>/idcdata/dmining/dminingoracle_ZHOBTMIND_togxpt.keyid</incrFileName>"
           "<timeout>30</timeout><procName>dminingoracle_ZHOBTMIND_togxpt</procName>"
           "<maxCount>1000</maxCount>"
           "<connectStr1>scott/tiger</connectStr1>\"\n\n");

    printf("本程序是共享平台的公共功能模块，用于从Oracle数据库源表抽取数据，生成xml文件。\n");
    printf("logFileName 本程序运行的日志文件。\n");
    printf("xmlBuffer   本程序运行的参数，用xml表示，具体如下：\n\n");

    printf("connectStr     数据源数据库的连接参数，格式：username/passwd@tnsname。\n");
    printf("charset     数据库的字符集，这个参数要与数据源数据库保持一致，否则会出现中文乱码的情况。\n");
    printf("selectSQL   从数据源数据库抽取数据的SQL语句，如果是增量抽取，一定要用递增字段作为查询条件，如where keyid>:1。\n");
    printf("fieldStr    抽取数据的SQL语句输出结果集的字段名列表，中间用逗号分隔，将作为xml文件的字段名。\n");
    printf("fieldLen    抽取数据的SQL语句输出结果集字段的长度列表，中间用逗号分隔。fieldStr与fieldLen的字段必须一一对应。\n");
    printf("outpath     输出xml文件存放的目录。\n");
    printf("bFileName   输出xml文件的前缀。\n");
    printf("eFileName   输出xml文件的后缀。\n");
    printf("maxCount    输出xml文件的最大记录数，缺省是0，表示无限制，如果本参数取值为0，注意适当加大timeout的取值，防止程序超时。\n");
    printf("startTime   程序运行的时间区间，例如02,13表示：如果程序启动时，踏中02时和13时则运行，其它时间不运行。"
           "如果startTime为空，表示不启用，只要本程序启动，就会执行数据抽取任务，为了减少数据源数据库压力"
           "抽取数据的时候，如果对时效性没有要求，一般在数据源数据库空闲的时候时进行。\n");
    printf("incrField    递增字段名，它必须是fieldStr中的字段名，并且只能是整型，一般为自增字段。"
           "如果incrField为空，表示不采用增量抽取的方案。");
    printf("incrFileName 已抽取数据的递增字段最大值存放的文件，如果该文件丢失，将重新抽取全部的数据。\n");
    printf("connectStr1    已抽取数据的递增字段最大值存放的数据库的连接参数。connectStr1和incrFileName二选一，connectStr1优先。");
    printf("timeout     本程序的超时时间，单位：秒。\n");
    printf("procName       进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n\n");
}
/**
 * @brief        : 程序退出和信号2、15的处理函数。
 * @param         {int} sig:
 * @return        {*}
 **/
void EXIT(int sig)
{
    logFile.write("程序退出，sig=%d\n\n", sig);

    exit(0);
}
