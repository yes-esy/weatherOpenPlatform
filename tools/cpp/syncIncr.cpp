/**
 * @FilePath     : /project/dataOpenPlatform/tools/cpp/syncIncr.cpp
 * @Description  :  本程序是共享平台的公共功能模块，采用增量的方法同步Oracle数据库之间的表。
 * @Author       : shengYang 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : shengYang 2900226123@qq.com
 * @LastEditTime : 2025-10-22 22:27:49
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/

#include "_tools.h"
/**
 * 程序运行参数
 **/
struct runArg_t
{
    char localConnStr[101];    // 本地数据库的连接参数。
    char charset[51];          // 数据库的字符集。
    char linkTableName[31];    // dblink指向的远程表名，如T_ZHOBTCODE1@db128。
    char localTableName[31];   // 本地表名。
    char remoteColsName[1001]; // 远程表的字段列表。
    char localColsName[1001];  // 本地表的字段列表。
    char where[1001];          // 同步数据的条件。
    char remoteConnStr[101];   // 远程数据库的连接参数。
    char remoteTableName[31];  // 远程表名。
    char remoteKeyCol[31];     // 远程表的键值字段名。
    char localKeyCol[31];      // 本地表的键值字段名。
    int maxCount;              // 每批执行一次同步操作的记录数。
    int timeInterval;          // 同步时间间隔，单位：秒，取值1-30。
    int timeout;               // 本程序运行时的超时时间。
    char procName[51];         // 本程序运行时的程序名。
} runArg;
void _help();                             // 显示程序的帮助
bool _xmltoarg(const char *strXMLBuffer); // 把xml解析到参数runArg结构中
clogfile logFile;                         // 日志文件
connection localConn;                     // 本地数据库连接。
connection remoteConn;                    // 远程数据库连接
bool _syncIncr(bool &isContinue);         // 业务处理主函数。
long maxKeyValue = 0;                     // 从本地表runArg.localTableName获取自增字段的最大值，存放在maxKeyValue全局变量中。
bool loadMaxKey();
void EXIT(int sig);  // 信号2和15处理函数
cpactive procActive; // 进程心跳
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        _help();
        return -1;
    }

    // 关闭全部的信号和输入输出，处理程序退出的信号。
    // closeioandsignal(true);
    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);

    if (logFile.open(argv[1]) == false)
    {
        printf("open log file %s failed.\n", argv[1]);
        return -1;
    }

    // 把xml解析到参数runArg结构中
    if (_xmltoarg(argv[2]) == false)
        return -1;
    // pactive.addpinfo(runArg.timeout,runArg.procName);
    // 注意，在调试程序的时候，可以启用类似以下的代码，防止超时。
    procActive.addpinfo(runArg.timeout * 10000, runArg.procName);

    if (localConn.connecttodb(runArg.localConnStr, runArg.charset) != 0)
    {
        logFile.write("connect local database failed,connect string : %s .\n error message:%s\n", runArg.localConnStr, localConn.message());
        EXIT(-1);
    }
    logFile.write("connect local database succeessfully.\n");
    if (remoteConn.connecttodb(runArg.remoteConnStr, runArg.charset) != 0)
    {
        logFile.write("connect remote database failed,connect string : %s .\n error message:%s\n", runArg.localConnStr, localConn.message());
        EXIT(-1);
    }
    logFile.write("connect remote database succeessfully.\n");
    // 如果runArg.remoteColsName或runArg.localColsName为空，就用runArg.localTableName表的全部列来填充。
    if ((strlen(runArg.remoteColsName) == 0) || (strlen(runArg.localColsName) == 0))
    {
        ObtainColumnsInfo obtainCols;

        // 获取runArg.localTableName表的全部列。
        if (obtainCols.getAllColumnsInfo(localConn, runArg.localTableName) == false)
        {
            logFile.write("table %s unexist\n", runArg.localTableName);
            EXIT(-1);
        }

        if (strlen(runArg.remoteColsName) == 0)
            strcpy(runArg.remoteColsName, obtainCols.allColumnsName.c_str());
        if (strlen(runArg.localColsName) == 0)
            strcpy(runArg.localColsName, obtainCols.allColumnsName.c_str());
    }
    bool isContinue;
    // 业务处理主函数。
    while (true)
    {
        if (_syncIncr(isContinue) == false)
        {
            EXIT(-1);
        }
        if (isContinue == false)
        {
            sleep(runArg.timeInterval);
        }
        procActive.uptatime();
    }
}
/**
 * @brief        : 显示程序的帮助
 * @return        {*}
 **/
void _help()
{
    printf("Using:/project/dataOpenPlatform/tools/bin/syncRef logFilename xmlbuffer\n\n");
    printf("Sample:/project/dataOpenPlatform/tools/bin/procctl 10 /project/dataOpenPlatform/tools/bin/syncIncr /log/idc/synInc_ZHOBTCODE2.log "
           "\"<localConnStr>idc/idcpwd</localConnStr>"
           "<remoteConnStr>idc/idcpwd@snorcl11g_128</remoteConnStr>"
           "<charset>Simplified Chinese_China.AL32UTF8</charset>"
           "<linkTableName>T_ZHOBTMIND1@db128</linkTableName>"
           "<remoteTableName>T_ZHOBTMIND1</remoteTableName>"
           "<localTableName>T_ZHOBTMIND2</localTableName>"
           "<remoteColsName>site_id,visit_date,t,p,u,wd,wf,r,vis,update_time,key_id</remoteColsName>"
           "<localColsName>site_id,visit_date,t,p,u,wd,wf,r,vis,update_time,key_id</localColsName>"
           "<remoteKeyCol>key_id</remoteKeyCol>"
           "<localKeyCol>key_id</localKeyCol>"
           "<timeout>50</timeout>"
           "<maxCount>50</maxCount>"
           "<timeInterval>2</timeInterval>"
           "<procName>syncRef_ZHOBTCODE3</procName>\"\n\n");

    printf("把T_ZHOBTMIND1@db128表中满足\"and site_id like '57%%'\"的记录增量同步到T_ZHOBTMIND3中。\n");
    printf("       /project/tools/bin/procctl 10 /project/tools/bin/syncinc /log/idc/syncinc_ZHOBTMIND3.log "
           "\"<localConnStr>idc/idcpwd@snorcl11g_128</localConnStr>"
           "<remoteConnStr>idc/idcpwd@snorcl11g_128</remoteConnStr>"
           "<charset>Simplified Chinese_China.AL32UTF8</charset>"
           "<remoteTableName>T_ZHOBTMIND1</remoteTableName>"
           "<linkTableName>T_ZHOBTMIND1@db128</linkTableName>"
           "<localTableName>T_ZHOBTMIND3</localTableName>"
           "<remoteColsName>site_id,visit_date,t,p,u,wd,wf,r,vis,update_time,key_id</remoteColsName>"
           "<localColsName>site_id,visit_date,t,p,u,wd,wf,r,vis,update_time,key_id</localColsName>"
           "<where>and key_id like '54%%%%'</where>"
           "<remoteKeyCol>key_id</remoteKeyCol>"
           "<localKeyCol>key_id</localKeyCol>"
           "<maxCount>300</maxCount>"
           "<timeInterval>2</timeInterval>"
           "<timeout>30</timeout>"
           "<procName>syncinc_ZHOBTMIND3</procName>\"\n\n");

    printf("本程序是共享平台的公共功能模块，采用增量的方法同步Oracle数据库之间的表。\n\n");

    printf("logfilename   本程序运行的日志文件。\n");
    printf("xmlbuffer     本程序运行的参数，用xml表示，具体如下：\n\n");

    printf("localConnStr  本地数据库的连接参数，格式：username/passwd@tnsname。\n");
    printf("charset       数据库的字符集，这个参数要与远程数据库保持一致，否则会出现中文乱码的情况。\n");

    printf("linkTableName      远程表名，在remoteTableName参数后加@dblink。\n");
    printf("localTableName    本地表名。\n");

    printf("remoteColsName    远程表的字段列表，用于填充在select和from之间，所以，remoteColsName可以是真实的字段，"
           "也可以是函数的返回值或者运算结果。如果本参数为空，就用localTableName表的字段列表填充。\n");
    printf("localColsName     本地表的字段列表，与remoteColsName不同，它必须是真实存在的字段。如果本参数为空，"
           "就用localTableName表的字段列表填充。\n");

    printf("where         同步数据的条件，填充在select starg.remotekeycol from remoteTableName where starg.remotekeycol>:1之后，"
           "注意，不要加where关键字，但是，需要加and关键字，本参数可以为空。\n");

    printf("remoteConnStr 远程数据库的连接参数，格式与localConnStr相同。\n");
    printf("remoteTableName   远程表名，表名后面不要加dblink。\n");
    printf("remoteKeyCol  远程表的自增字段名。\n");
    printf("localKeyCol   本地表的自增字段名。\n");

    printf("maxCount      每批操作的记录数，建议在100-500之间。\n");

    printf("timeIntertval       执行同步任务的时间间隔，单位：秒，取值1-30。\n");
    printf("timeout       本程序的超时时间，单位：秒，视数据量的大小而定，建议设置30以上。\n");
    printf("procName         本程序运行时的进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n\n");
}
/**
 * @brief        : 把xml解析到参数runArg结构中
 * @param         {char} *strXMLBuffer: xml文件缓冲区
 * @return        {bool} : 成功返回true,失败返回false
 **/
bool _xmltoarg(const char *strXMLBuffer)
{
    memset(&runArg, 0, sizeof(struct runArg_t));

    // 本地数据库的连接参数，格式：ip,username,password,dbname,port。
    getxmlbuffer(strXMLBuffer, "localConnStr", runArg.localConnStr, 100);
    if (strlen(runArg.localConnStr) == 0)
    {
        logFile.write("localConnStr is null.\n");
        return false;
    }

    // 数据库的字符集，这个参数要与远程数据库保持一致，否则会出现中文乱码的情况。
    getxmlbuffer(strXMLBuffer, "charset", runArg.charset, 50);
    if (strlen(runArg.charset) == 0)
    {
        logFile.write("charset is null.\n");
        return false;
    }

    // linkTableName表名。
    getxmlbuffer(strXMLBuffer, "linkTableName", runArg.linkTableName, 30);
    if (strlen(runArg.linkTableName) == 0)
    {
        logFile.write("linkTableName is null.\n");
        return false;
    }

    // 本地表名。
    getxmlbuffer(strXMLBuffer, "localTableName", runArg.localTableName, 30);
    if (strlen(runArg.localTableName) == 0)
    {
        logFile.write("localTableName is null.\n");
        return false;
    }

    // 远程表的字段列表，用于填充在select和from之间，所以，remoteColsName可以是真实的字段，也可以是函数
    // 的返回值或者运算结果。如果本参数为空，将用localTableName表的字段列表填充。
    getxmlbuffer(strXMLBuffer, "remoteColsName", runArg.remoteColsName, 1000);

    // 本地表的字段列表，与remoteColsName不同，它必须是真实存在的字段。如果本参数为空，将用localTableName表的字段列表填充。
    getxmlbuffer(strXMLBuffer, "localColsName", runArg.localColsName, 1000);

    // 同步数据的条件。
    getxmlbuffer(strXMLBuffer, "where", runArg.where, 1000);

    // 同步方式：1-不分批刷新；2-分批刷新。
    getxmlbuffer(strXMLBuffer, "remoteConnStr", runArg.remoteConnStr);
    if (strlen(runArg.remoteConnStr) == 0)
    {
        logFile.write("remote Connection String is null.\n");
        return false;
    }
    // 远程表名。
    getxmlbuffer(strXMLBuffer, "remoteTableName", runArg.remoteTableName, 30);
    if (strlen(runArg.remoteTableName) == 0)
    {
        logFile.write("remote Table Name is null.\n");
        return false;
    }

    // 远程表的自增字段名。
    getxmlbuffer(strXMLBuffer, "remoteKeyCol", runArg.remoteKeyCol, 30);
    if (strlen(runArg.remoteKeyCol) == 0)
    {
        logFile.write("remoteKeyCol is null.\n");
        return false;
    }

    // 本地表的自增字段名。
    getxmlbuffer(strXMLBuffer, "localKeyCol", runArg.localKeyCol, 30);
    if (strlen(runArg.localKeyCol) == 0)
    {
        logFile.write("localKeyCol is null.\n");
        return false;
    }

    // 每批执行一次同步操作的记录数。
    getxmlbuffer(strXMLBuffer, "maxCount", runArg.maxCount);
    if (runArg.maxCount == 0)
    {
        logFile.write("maxCount is null.\n");
        return false;
    }

    // 执行同步的时间间隔，单位：秒，取值1-30。
    getxmlbuffer(strXMLBuffer, "timeInterval", runArg.timeInterval);
    if (runArg.timeInterval <= 0)
    {
        logFile.write("timeInterval is null.\n");
        return false;
    }
    if (runArg.timeInterval > 30)
        runArg.timeInterval = 30; // 没必要超过30秒。

    // 本程序的超时时间，单位：秒，视数据量的大小而定，建议设置30以上。
    getxmlbuffer(strXMLBuffer, "timeout", runArg.timeout);
    if (runArg.timeout == 0)
    {
        logFile.write("timeout is null.\n");
        return false;
    }

    // 本程序运行时的进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。
    getxmlbuffer(strXMLBuffer, "procName", runArg.procName, 50);
    if (strlen(runArg.procName) == 0)
    {
        logFile.write("procName is null.\n");
        return false;
    }

    return true;
}
/**
 * @brief        : 加载最大的键值
 * @return        {*}
 **/
bool loadMaxKey()
{
    maxKeyValue = 0;

    sqlstatement selectStatement(&localConn);
    selectStatement.prepare("select max(%s) from %s", runArg.localKeyCol, runArg.localTableName);
    selectStatement.bindout(1, maxKeyValue);

    if (selectStatement.execute() != 0)
    {
        logFile.write("select SQL: %s execute failed,method :selectStatement.execute(), error message:%s\n", selectStatement.sql(), selectStatement.message());
        return false;
    }

    selectStatement.next();

    logFile.write("maxKeyValue=%ld\n", maxKeyValue);

    return true;
}
/**
 * @brief        : 业务处理主函数。
 * @param         {bool} &isContinue: 是否继续
 * @return        {*}
 **/
bool _syncIncr(bool &isContinue)
{
    ctimer timer;

    isContinue = false;

    // 从本地表starg.localtname获取自增字段的最大值，存放在maxKeyValue全局变量中。
    if (loadMaxKey() == false)
        return false;

    // 从远程表查找需要同步的记录（自增字段的值大于maxKeyValue的记录）。
    // select rowid from T_ZHOBTMIND1 where keyid>1461243 and obtid like '57%' order by keyid;
    char remoteRowIdValue[21]; // 从远程表查到的需要同步记录的rowid。
    sqlstatement selectStatement(&remoteConn);
    selectStatement.prepare("select rowid from %s where %s>:1 %s order by %s", runArg.remoteTableName, runArg.remoteKeyCol, runArg.where, runArg.remoteKeyCol);
    selectStatement.bindin(1, maxKeyValue);
    selectStatement.bindout(1, remoteRowIdValue, 20);

    // 拼接绑定插入SQL语句参数的字符串（:1,:2,:3,...,:runArg.maxcount）。
    string bindSqlStr; // 绑定插入SQL语句参数的字符串。
    for (int i = 0; i < runArg.maxCount; i++)
    {
        bindSqlStr = bindSqlStr + sformat(":%lu,", i + 1);
    }
    deleterchr(bindSqlStr, ','); // 最后一个逗号是多余的。

    char rowIdValues[runArg.maxCount][21]; // 存放rowid的值。

    // 准备插入本地表数据的SQL语句，一次插入runArg.maxCount条记录。
    // insert into T_ZHOBTMIND3(obtid,ddatetime,t,p,u,wd,wf,r,vis,upttime,keyid)
    //                   select obtid,ddatetime,t,p,u,wd,wf,r,vis,upttime,keyid from T_ZHOBTMIND1@db128
    //                    where rowid in (:1,:2,:3);
    sqlstatement insertStatement(&localConn); // 向本地表中插入数据的SQL语句。
    insertStatement.prepare("insert into %s(%s) select %s from %s where rowid in (%s)",
                            runArg.localTableName, runArg.localColsName, runArg.remoteColsName, runArg.linkTableName, bindSqlStr.c_str());
    for (int i = 0; i < runArg.maxCount; i++)
    {
        insertStatement.bindin(i + 1, rowIdValues[i]);
    }

    int count = 0; // 记录从结果集中已获取记录的计数器。

    memset(rowIdValues, 0, sizeof(rowIdValues));

    if (selectStatement.execute() != 0)
    {
        logFile.write("select SQL: %s execute failed,method :selectStatement.execute(), error message:%s\n", selectStatement.sql(), selectStatement.message());
        return false;
    }

    while (true)
    {
        // 获取需要同步数据的结果集。
        if (selectStatement.next() != 0)
            break;

        strcpy(rowIdValues[count], remoteRowIdValue);

        count++;

        // 每runArg.maxCount条记录执行一次同步。
        if (count == runArg.maxCount)
        {
            // 向本地表中插入记录。
            if (insertStatement.execute() != 0)
            {
                // 执行向本地表中插入记录的操作一般不会出错。
                // 如果报错，就肯定是数据库的问题或同步的参数配置不正确，流程不必继续。
                logFile.write("insert SQL: %s execute failed,method :selectStatement.execute(), error message:%s\n", insertStatement.sql(), insertStatement.message());

                return false;
            }

            // logFile.write("sync %s to %s(%d rows) in %.2fsec.\n",runArg.lnktname,runArg.localtname,count,timer.elapsed());

            localConn.commit();

            count = 0; // 记录从结果集中已获取记录的计数器。

            memset(rowIdValues, 0, sizeof(rowIdValues));

            procActive.uptatime();
        }
    }

    // 如果count>0，表示还有没同步的记录，再执行一次同步。
    if (count > 0)
    {
        // 向本地表中插入记录。
        if (insertStatement.execute() != 0)
        {
            logFile.write("insertStatement.execute() failed.\n%s\n%s\n", insertStatement.sql(), insertStatement.message());
            return false;
        }

        // logFile.write("sync %s to %s(%d rows) in %.2fsec.\n",runArg.lnktname,runArg.localtname,count,timer.elapsed());

        localConn.commit();
    }

    if (selectStatement.rpc() > 0)
    {
        logFile.write("sync %s to %s(%d rows) in %.2fsec.\n", runArg.linkTableName, runArg.localTableName, selectStatement.rpc(), timer.elapsed());
        isContinue = true;
    }

    return true;
}
/**
 * @brief        : 信号2和15处理函数
 * @param         {int} sig: 信号
 * @return        {void}
 **/
void EXIT(int sig)
{
    logFile.write("program quit,sig=%d\n\n", sig);

    localConn.disconnect();

    remoteConn.disconnect();
    exit(0);
}
