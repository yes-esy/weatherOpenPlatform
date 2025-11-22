/**
 * @FilePath     : /project/dataOpenPlatform/tools/cpp/syncRef.cpp
 * @Description  :  本程序是共享平台的公共功能模块，采用刷新的方法同步Oracle数据库之间的表。
 * @Author       : shengYang 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : shengYang 2900226123@qq.com
 * @LastEditTime : 2025-10-22 17:28:39
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
    char rWhere[1001];         // 同步数据的条件。
    char lWhere[1001];         // 同步数据的条件。
    int syncType;              // 同步方式：1-不分批刷新；2-分批刷新。
    char remoteConnStr[101];   // 远程数据库的连接参数。
    char remoteTableName[31];  // 远程表名。
    char remoteKeyCol[31];     // 远程表的键值字段名。
    char localKeyCol[31];      // 本地表的键值字段名。
    int keyLen;                // 键值字段的长度。
    int maxCount;              // 每批执行一次同步操作的记录数。
    int timeout;               // 本程序运行时的超时时间。
    char procName[51];         // 本程序运行时的程序名。
} runArg;
void _help();                             // 显示程序的帮助
bool _xmltoarg(const char *strxmlbuffer); // 把xml解析到参数runArg结构中
clogfile logFile;                         // 日志文件
connection localConn;                     // 本地数据库连接。
connection remoteConn;                    // 远程数据库连接
bool _syncRef();                          // 业务处理主函数。
void EXIT(int sig);                       // 信号2和15处理函数
cpactive pactive;                         // 进程心跳
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
    pactive.addpinfo(runArg.timeout * 10000, runArg.procName);

    if (localConn.connecttodb(runArg.localConnStr, runArg.charset) != 0)
    {
        logFile.write("connect local database failed,connect string : %s .\n error message:%s\n", runArg.localConnStr, localConn.message());
        EXIT(-1);
    }

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

    // 业务处理主函数。
    _syncRef();
}
/**
 * @brief        : 显示程序的帮助
 * @return        {*}
 **/
void _help()
{
    printf("Using:/project/dataOpenPlatform/tools/bin/syncRef logFilename xmlbuffer\n\n");

    // 不分批同步，把T_ZHOBTCODE1@db128同步到T_ZHOBTCODE2。
    printf("Sample:/project/dataOpenPlatform/tools/bin/procctl 10 /project/dataOpenPlatform/tools/bin/syncRef /log/idc/syncRef_ZHOBTCODE2.log "
           "\"<localConnStr>idc/idcpwd</localConnStr>"
           "<charset>Simplified Chinese_China.AL32UTF8</charset>"
           "<linkTableName>T_ZHOBTCODE@db128</linkTableName>"
           "<localTableName>T_ZHOBTCODE2</localTableName>"
           "<remoteColsName>site_id,city_name,province_name,latitude,longitude,height,update_time,key_id</remoteColsName>"
           "<localColsName>site_id,city_name,province_name,latitude,longitude,height,update_time,key_id</localColsName>"
           "<rWhere>where site_id like '57%%%%'</rWhere>"
           "<lWhere>where site_id like '57%%%%'</lWhere>"
           "<syncType>1</syncType>"
           "<timeout>50</timeout>"
           "<procName>syncRef_ZHOBTCODE2</procName>\"\n\n");

    // 分批同步，把T_ZHOBTCODE1@db128同步到T_ZHOBTCODE3。
    // 因为测试的需要，xmltodb程序每次会删除T_ZHOBTCODE1@db128中的数据，全部的记录重新入库，keyid会变。
    // 所以以下脚本不能用keyid，要用obtid，用keyid会出问题，可以试试。
    printf("       /project/dataOpenPlatform/tools/bin/procctl 10 /project/dataOpenPlatform/tools/bin/syncRef /log/idc/syncRef_ZHOBTCODE3.log "
           "\"<localConnStr>idc/idcpwd</localConnStr>"
           "<charset>Simplified Chinese_China.AL32UTF8</charset>"
           "<linkTableName>T_ZHOBTCODE2@db128</linkTableName>"
           "<localTableName>T_ZHOBTCODE3</localTableName>"
           "<remoteColsName>site_id,city_name,province_name,latitude,longitude,height,update_time,key_id</remoteColsName>"
           "<localColsName>site_id,city_name,province_name,latitude,longitude,height,update_time,key_id</localColsName>"
           "<rWhere>where site_id like '57%%%%'</rWhere>"
           "<lWhere>where site_id like '57%%%%'</lWhere>"
           "<syncType>2</syncType>"
           "<remoteConnStr>idc/idcpwd@snorcl11g_128</remoteConnStr>"
           "<remoteTableName>T_ZHOBTCODE2</remoteTableName>"
           "<remoteKeyCol>site_id</remoteKeyCol>"
           "<localKeyCol>site_id</localKeyCol>"
           "<keyLen>5</keyLen>"
           "<maxCount>10</maxCount>"
           "<timeout>50</timeout>"
           "<procName>syncRef_ZHOBTCODE3</procName>\"\n\n");

    // 分批同步，把T_ZHOBTMIND1@db128同步到T_ZHOBTMIND2。
    printf("       /project/dataOpenPlatform/tools/bin/procctl 10 /project/dataOpenPlatform/tools/bin/syncRef /log/idc/syncRef_ZHOBTMIND2.log "
           "\"<localConnStr>idc/idcpwd</localConnStr>"
           "<charset>Simplified Chinese_China.AL32UTF8</charset>"
           "<linkTableName>T_ZHOBTMIND1@db128</linkTableName>"
           "<localTableName>T_ZHOBTMIND2</localTableName>"
           "<remoteColsName>obtid,ddatetime,t,p,u,wd,wf,r,vis,upttime,keyid</remoteColsName>"
           "<localColsName>stid,ddatetime,t,p,u,wd,wf,r,vis,upttime,recid</localColsName>"
           "<where>where ddatetime>sysdate-10/1440</where>"
           "<syncType>2</syncType>"
           "<remoteConnStr>idc/idcpwd</remoteConnStr>"
           "<remoteTableName>T_ZHOBTMIND1</remoteTableName>"
           "<remoteKeyCol>keyid</remoteKeyCol>"
           "<localKeyCol>recid</localKeyCol>"
           "<keyLen>15</keyLen>"
           "<maxCount>10</maxCount>"
           "<timeout>50</timeout>"
           "<procName>syncRef_ZHOBTMIND2</procName>\"\n\n");

    printf("本程序是共享平台的公共功能模块，采用刷新的方法同步Oracle数据库之间的表。\n\n");

    printf("logFilename   本程序运行的日志文件。\n");
    printf("xmlbuffer     本程序运行的参数，用xml表示，具体如下：\n\n");

    printf("localConnStr  本地数据库的连接参数，格式：username/passwd@tnsname。\n");
    printf("charset       数据库的字符集，这个参数要与本地和远程数据库保持一致，否则会出现中文乱码的情况。\n");

    printf("linkTableName      dblink指向的远程表名，如T_ZHOBTCODE1@db128。\n");
    printf("localTableName    本地表名，如T_ZHOBTCODE2。\n");

    printf("remoteColsName    远程表的字段列表，用于填充在select和from之间，所以，remoteColsName可以是真实的字段，"
           "也可以是函数的返回值或者运算结果。如果本参数为空，就用localTableName表的字段列表填充。\n");
    printf("localColsName     本地表的字段列表，与remoteColsName不同，它必须是真实存在的字段。如果本参数为空，"
           "就用localTableName表的字段列表填充。\n");

    printf("rWhere        同步数据的条件，填充在远程表的查询语句之后，为空则表示同步全部的记录。\n");
    printf("lWhere        同步数据的条件，填充在本地表的删除语句之后，为空则表示同步全部的记录。\n");

    printf("syncType      同步方式：1-不分批刷新；2-分批刷新。\n");
    printf("remoteConnStr 远程数据库的连接参数，格式与localConnStr相同，当syncType==2时有效。\n");
    printf("remoteTableName   远程表名，当syncType==2时有效。\n");
    printf("remoteKeyCol  远程表的键值字段名，必须是唯一的，当syncType==2时有效。\n");
    printf("localKeyCol   本地表的键值字段名，必须是唯一的，当syncType==2时有效。\n");
    printf("keyLen        键值字段的长度，当syncType==2时有效。\n");

    printf("maxCount      每批执行一次同步操作的记录数，当syncType==2时有效。\n");

    printf("timeout       本程序的超时时间，单位：秒，视数据量的大小而定，建议设置30以上。\n");
    printf("procName         本程序运行时的进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n");
    printf("注意：\n"
           "1）remoteKeyCol和localKeyCol字段的选取很重要，如果是自增字段，那么在远程表中数据生成后自增字段的值不可改变，否则同步会失败；\n"
           "2）当远程表中存在delete操作时，无法分批刷新，因为远程表的记录被delete后就找不到了，无法从本地表中执行delete操作。\n\n\n");
}
/**
 * @brief        : 把xml解析到参数runArg结构中
 * @param         {char} *strxmlbuffer: xml文件缓冲区
 * @return        {bool} : 成功返回true,失败返回false
 **/
bool _xmltoarg(const char *strxmlbuffer)
{
    memset(&runArg, 0, sizeof(struct runArg_t));

    // 本地数据库的连接参数，格式：ip,username,password,dbname,port。
    getxmlbuffer(strxmlbuffer, "localConnStr", runArg.localConnStr, 100);
    if (strlen(runArg.localConnStr) == 0)
    {
        logFile.write("localConnStr is null.\n");
        return false;
    }

    // 数据库的字符集，这个参数要与远程数据库保持一致，否则会出现中文乱码的情况。
    getxmlbuffer(strxmlbuffer, "charset", runArg.charset, 50);
    if (strlen(runArg.charset) == 0)
    {
        logFile.write("charset is null.\n");
        return false;
    }

    // linkTableName表名。
    getxmlbuffer(strxmlbuffer, "linkTableName", runArg.linkTableName, 30);
    if (strlen(runArg.linkTableName) == 0)
    {
        logFile.write("linkTableName is null.\n");
        return false;
    }

    // 本地表名。
    getxmlbuffer(strxmlbuffer, "localTableName", runArg.localTableName, 30);
    if (strlen(runArg.localTableName) == 0)
    {
        logFile.write("localTableName is null.\n");
        return false;
    }

    // 远程表的字段列表，用于填充在select和from之间，所以，remoteColsName可以是真实的字段，也可以是函数
    // 的返回值或者运算结果。如果本参数为空，将用localTableName表的字段列表填充。
    getxmlbuffer(strxmlbuffer, "remoteColsName", runArg.remoteColsName, 1000);

    // 本地表的字段列表，与remoteColsName不同，它必须是真实存在的字段。如果本参数为空，将用localTableName表的字段列表填充。
    getxmlbuffer(strxmlbuffer, "localColsName", runArg.localColsName, 1000);

    // 同步数据的条件。
    getxmlbuffer(strxmlbuffer, "rWhere", runArg.rWhere, 1000);
    getxmlbuffer(strxmlbuffer, "lWhere", runArg.lWhere, 1000);

    // 同步方式：1-不分批刷新；2-分批刷新。
    getxmlbuffer(strxmlbuffer, "syncType", runArg.syncType);
    if ((runArg.syncType != 1) && (runArg.syncType != 2))
    {
        logFile.write("syncType is not in (1,2).\n");
        return false;
    }

    if (runArg.syncType == 2)
    {
        // 远程数据库的连接参数，格式与localConnStr相同，当syncType==2时有效。
        getxmlbuffer(strxmlbuffer, "remoteConnStr", runArg.remoteConnStr, 100);
        if (strlen(runArg.remoteConnStr) == 0)
        {
            logFile.write("remoteConnStr is null.\n");
            return false;
        }

        // 远程表名，当syncType==2时有效。
        getxmlbuffer(strxmlbuffer, "remoteTableName", runArg.remoteTableName, 30);
        if (strlen(runArg.remoteTableName) == 0)
        {
            logFile.write("remoteTableName is null.\n");
            return false;
        }

        // 远程表的键值字段名，必须是唯一的，当syncType==2时有效。
        getxmlbuffer(strxmlbuffer, "remoteKeyCol", runArg.remoteKeyCol, 30);
        if (strlen(runArg.remoteKeyCol) == 0)
        {
            logFile.write("remoteKeyCol is null.\n");
            return false;
        }

        // 本地表的键值字段名，必须是唯一的，当syncType==2时有效。
        getxmlbuffer(strxmlbuffer, "localKeyCol", runArg.localKeyCol, 30);
        if (strlen(runArg.localKeyCol) == 0)
        {
            logFile.write("localKeyCol is null.\n");
            return false;
        }

        // 键值字段的大小。
        getxmlbuffer(strxmlbuffer, "keyLen", runArg.keyLen);
        if (runArg.keyLen == 0)
        {
            logFile.write("keyLen is null.\n");
            return false;
        }

        // 每批执行一次同步操作的记录数，当syncType==2时有效。
        getxmlbuffer(strxmlbuffer, "maxCount", runArg.maxCount);
        if (runArg.maxCount == 0)
        {
            logFile.write("maxCount is null.\n");
            return false;
        }
    }

    // 本程序的超时时间，单位：秒，视数据量的大小而定，建议设置30以上。
    getxmlbuffer(strxmlbuffer, "timeout", runArg.timeout);
    if (runArg.timeout == 0)
    {
        logFile.write("timeout is null.\n");
        return false;
    }

    // 本程序运行时的进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。
    getxmlbuffer(strxmlbuffer, "procName", runArg.procName, 50);
    if (strlen(runArg.procName) == 0)
    {
        logFile.write("procName is null.\n");
        return false;
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

    exit(0);
}
/**
 * @brief        : 业务处理主函数。
 * @return        {bool}:成功-true,失败-false
 **/
bool _syncRef()
{
    ctimer timer;

    sqlstatement deleteStatement(&localConn); // 删除本地表中记录的SQL语句。
    sqlstatement insertStatement(&localConn); // 向本地表中插入数据的SQL语句。
    // 不分批刷新，适用于数据量不大（百万行以下）的场景，远程表中的增加、修改和删除操作都可以同步到本地表。
    // delete from T_ZHOBTCODE2 where stid like '57%';
    // insert into T_ZHOBTCODE2(stid,cityname,provname,lat,lon,height,upttime,recid)
    //        select obtid,cityname,provname,lat,lon,height,upttime,keyid from T_ZHOBTCODE1@db128 where obtid like '57%';
    if (runArg.syncType == 1)
    {
        logFile.write("sync %s to %s ...", runArg.linkTableName, runArg.localTableName);

        // 先删除本地表runArg.localTableName中满足runArg.lWhere条件的记录。
        deleteStatement.prepare("delete from %s %s", runArg.localTableName, runArg.lWhere);
        if (deleteStatement.execute() != 0)
        {
            logFile.write("delete SQL : %s execute failed ,method :deleteStatement.execute() ,error message :\n%s\n", deleteStatement.sql(), deleteStatement.message());
            return false;
        }

        // 再把远程表runArg.linkTableName中满足start.rWhere条件的记录插入到本地表runArg.localTableName。
        insertStatement.prepare("insert into %s(%s) select %s from %s %s", runArg.localTableName, runArg.localColsName, runArg.remoteColsName, runArg.linkTableName, runArg.rWhere);
        if (insertStatement.execute() != 0)
        {
            logFile.write("insert SQL : %s execute failed ,method :insertStatement.execute() ,error message :%s\n", insertStatement.sql(), insertStatement.message());

            localConn.rollback(); // 如果这里失败了，可以不用回滚事务，connection类的析构函数会回滚。
            return false;
        }

        logFile << " " << insertStatement.rpc() << " rows in " << timer.elapsed() << "sec.\n";

        localConn.commit();

        return true;
    }

    // 分批刷新
    if (remoteConn.connecttodb(runArg.remoteConnStr, runArg.charset) != 0)
    {
        logFile.write("connect remote database failed, connect string :%s,error message:%s\n", runArg.remoteConnStr, remoteConn.message());
        return false;
    }
    sqlstatement selectStatement(&remoteConn);
    selectStatement.prepare("select %s from %s %s", runArg.remoteKeyCol, runArg.remoteTableName, runArg.rWhere);
    char remoteKeyValues[runArg.keyLen + 1];
    selectStatement.bindout(1, remoteKeyValues, 50);
    string bindSqlStr;
    for (int i = 0; i < runArg.maxCount; i++)
    {
        bindSqlStr += sformat(":%lu,", i + 1);
    }
    deleterchr(bindSqlStr, ','); // 删除最后一个逗号
    char keyValues[runArg.maxCount][runArg.keyLen + 1];
    deleteStatement.prepare("delete from %s where %s in (%s)", runArg.localTableName, runArg.localKeyCol, bindSqlStr.c_str());
    for (int i = 0; i< runArg.maxCount; i++)
        deleteStatement.bindin(i + 1, keyValues[i]);

    insertStatement.prepare("insert into %s(%s) select %s from %s where %s in(%s)",
                            runArg.localTableName, runArg.localColsName, runArg.remoteColsName, runArg.remoteTableName, runArg.remoteKeyCol, bindSqlStr.c_str());
    for (int i = 0; i < runArg.maxCount; i++)
        insertStatement.bindin(i + 1, keyValues[i]);

    int count = 0;
    memset(keyValues, 0, sizeof(keyValues));
    logFile.write("sync %s to %s ...", runArg.linkTableName, runArg.localTableName);
    if (selectStatement.execute() != 0)
    {
        logFile.write("select SQL : %s execute failed ,method :insertStatement.execute() ,error message :%s\n", selectStatement.sql(), selectStatement.message());
        return false;
    }
    while (true)
    {
        // 获取需要同步数据的结果集。
        if (selectStatement.next() != 0)
            break;
        strcpy(keyValues[count], remoteKeyValues);
        count++;
        // 每starg.maxcount条记录执行一次同步操作。
        if (count == runArg.maxCount)
        {
            // 从本地表中删除记录。
            if (deleteStatement.execute() != 0)
            {
                // 执行从本地表中删除记录的操作一般不会出错。
                // 如果报错，就肯定是数据库的问题或同步的参数配置不正确，流程不必继续。
                logFile.write("delete SQL : %s execute failed ,method :deleteStatement.execute() ,error message :%s\n", deleteStatement.sql(), deleteStatement.message());
                return false;
            }
            // 向本地表中插入记录。
            if (insertStatement.execute() != 0)
            {
                // 执行向本地表中插入记录的操作一般不会出错。
                // 如果报错，就肯定是数据库的问题或同步的参数配置不正确，流程不必继续。
                logFile.write("insert SQL : %s execute failed ,method :insertStatement.execute() ,error message :%s\n", insertStatement.sql(), insertStatement.message());
                return false;
            }
            // logFile.write("sync %s to %s(%d rows) in %.2fsec.\n",starg.linktname,starg.localtname,ccount,timer.elapsed());
            localConn.commit();
            count = 0; // 记录从结果集中已获取记录的计数器。
            memset(keyValues, 0, sizeof(keyValues));
            pactive.uptatime();
        }
    }
    // 如果ccount>0，表示还有没同步的记录，再执行一次同步操作。
    if (count > 0)
    {
        // 从本地表中删除记录。
        if (deleteStatement.execute() != 0)
        {
            logFile.write("delete SQL : %s execute failed ,method :deleteStatement.execute() ,error message :%s\n", deleteStatement.sql(), deleteStatement.message());
            return false;
        }
        // 向本地表中插入记录。
        if (insertStatement.execute() != 0)
        {
            logFile.write("insert SQL : %s execute failed ,method :insertStatement.execute() ,error message :%s\n", insertStatement.sql(), insertStatement.message());
            return false;
        }
        logFile.write("sync %s to %s(%d rows) in %.2fsec.\n", runArg.linkTableName, runArg.localTableName, count, timer.elapsed());
        localConn.commit();
    }
    logFile << " " << selectStatement.rpc() << " rows in " << timer.elapsed() << "sec.\n";
    return true;
}
