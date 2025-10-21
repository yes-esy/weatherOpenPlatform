/**
 * @FilePath     : /project/dataOpenPlatform/tools/cpp/xmlToDB.cpp
 * @Description  :  将xml文件中的数据入库
 * @Author       : shengYang 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : shengYang 2900226123@qq.com
 * @LastEditTime : 2025-10-21 15:19:06
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "_tools.h"
using namespace idc;
/**
 * 程序运行参数的结构体。
 */
struct runArg_t
{
    char connectStr[101];  // 数据库的连接参数。
    char charset[51];      // 数据库的字符集。
    char iniFileName[301]; // 数据入库的参数配置文件。
    char xmlPath[301];     // 待入库xml文件存放的目录。
    char xmlPathBak[301];  // xml文件入库后的备份目录。
    char xmlPathErr[301];  // 入库失败的xml文件存放的目录。
    int timeInterval;      // 本程序运行的时间间隔，本程序常驻内存。
    int timeout;           // 本程序运行时的超时时间。
    char procName[51];     // 本程序运行时的程序名。
} runArg;
/**
 * 数据入库参数的结构体。
 */
struct xmlToDBArg_t
{
    char fileName[101]; // xml文件的匹配规则，用逗号分隔。
    char tableName[31]; // 待入库的表名。
    int updateFlag;     // 更新标志：1-更新；2-不更新。
    char execSQL[301];  // 处理xml文件之前，执行的SQL语句。
} xmlToDBArg;
void _help(char *argv[]);                                                                 // 程序的帮助文档
bool _xmlToRunArg(const char *stringXMLBuffer);                                           // 把xml解析到参数runArg结构中
vector<struct xmlToDBArg_t> xmlToTableV;                                                  // 数据入库的参数的容器。
bool loadXmlToTable();                                                                    // 把数据入库的参数配置文件runArg.inifilename加载到vxmltotable容器中。
bool findXmlToTable(const string &xmlFileName);                                           // 根据文件名，从vxmltotable容器中查找的入库参数，存放在stxmltotable结构体中。
clogfile logFile;                                                                         // 本程序运行的日志。
connection oracleConn;                                                                    // 数据库连接。
ObtainColumnsInfo obtainCols;                                                             // 获取数据库表字段类
string insertSQLStr;                                                                      // 插入表的SQL
string updateSQLStr;                                                                      // 更新表的SQL
sqlstatement insertStatement, updateStatement;                                            // 插入和更新表的sqlstatement语句
vector<string> columnValueV;                                                              // 存放从每一行xml文件中解析出来的字段的值用于绑定插入和更新表的SQL语句
ctimer timer;                                                                             // 用于记录每一个文件的耗时
int totalCnt, insertCnt, updateCnt;                                                       // xml总记录数,更新记录数和插入记录数
cpactive procAct;                                                                         // 进程心跳全局对象
void jointSQL();                                                                          // 拼接插入和更新的SQL
void prepareSQL();                                                                        // 准备SQL语句,绑定输入变量
bool executeSQL();                                                                        // 执行sql
void splitBuffer(const string &stringBuffer);                                             // 解析出数据库每个字段的值
void EXIT(int sig);                                                                       // 程序退出的信号处理函数。
bool _xmlToDB();                                                                          // 业务处理主函数。
int _xmlToDB(const string &fullFileName, const string &fileName);                         // 处理xml文件的子函数，返回值：0-成功，其它的都是失败，失败的情况有很多种，暂时不确定。
bool xmlToBak(const string &fullFileName, const string &srcPath, const string &destPath); // 移动到备份目录
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        _help(argv);
        return false;
    }
    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);
    if (logFile.open(argv[1]) == false)
    {
        printf("打开日志文件失败（%s）。\n", argv[1]);
        return -1;
    }

    // 把xml解析到参数runArg结构中
    if (_xmlToRunArg(argv[2]) == false)
    {
        logFile.write("xml arg load failed.\n");
        EXIT(-1);
    }
    _xmlToDB();
    procAct.addpinfo(runArg.timeout, runArg.procName); // 设置进程心跳
    return 0;
}
/**
 * @brief        : 将xml文件移动到备份目录
 * @param         {string} &fullFileName: 需要备份的文件名
 * @param         {string} &srcPath: 源路径
 * @param         {string} &destPath: 目的路径
 * @return        {bool} : 成功返回true,失败false
 **/
bool xmlToBak(const string &fullFileName, const string &srcPath, const string &destPath)
{
    string destFileName = fullFileName;
    replacestr(destFileName, srcPath, destPath, false);
    if (renamefile(fullFileName, destFileName.c_str()) == false)
    {
        logFile.write("rename file failed,method:renamefile(fullFileName,destFileName.c_str()),arg:(%s,%s)\n", fullFileName.c_str(), destFileName.c_str());
        return false;
    }
    return true;
}
/**
 * @brief        : 预处理列名将列名转换成驼峰命名的形式
 * @param         {string} &columnName: 源列名
 * @return        {string} : 转换后的驼峰命名
 **/
string preProceedColumnName(const string &columnName)
{
    string newColumnName = "";
    for (size_t i = 0; i < columnName.length(); ++i)
    {
        if (columnName[i] == '_')
        {
            continue;
        }
        else if (i > 0 && columnName[i - 1] == '_')
        {
            newColumnName += toupper(columnName[i]);
        }
        else
        {
            newColumnName += columnName[i];
        }
    }
    return newColumnName;
}
/**
 * @brief        : 解析出数据库每个字段的值到columnValueV中
 * @param         {string} &stringBuffer: 带解析的字符串
 * @return        {void}
 **/
void splitBuffer(const string &stringBuffer)
{

    string strTemp;
    for (int i = 0; i < obtainCols.allColumnsInfo.size(); i++)
    {
        // 根据字段名把数据项中的值解析出来,存放在临时变量strTemp中.
        string newColumnName = preProceedColumnName(obtainCols.allColumnsInfo[i].columnName);
        getxmlbuffer(stringBuffer, newColumnName, strTemp, obtainCols.allColumnsInfo[i].columnLen);
        if (strcmp(obtainCols.allColumnsInfo[i].dataType, "date") == 0) // 是date类型
        {
            picknumber(strTemp, strTemp, false, false);
        }
        else if (strcmp(obtainCols.allColumnsInfo[i].dataType, "number") == 0)
        {
            picknumber(strTemp, strTemp, true, true);
        }
        columnValueV[i] = strTemp.c_str();
    }
    return;
}
/**
 * @brief        : 执行sql
 * @return        {bool} : 执行成功返回true,失败返回false
 **/
bool executeSQL()
{
    if (strlen(xmlToDBArg.execSQL) == 0)
    {
        return false;
    }
    sqlstatement statement;
    statement.connect(&oracleConn);
    statement.prepare(xmlToDBArg.execSQL);
    if (statement.execute() != 0)
    {
        logFile.write("SQL:%s execute failed, error message: %s \n", statement.sql(), statement.message());
        return false;
    }
    return true;
}
/**
 * @brief        : 准备SQL语句,绑定输入变量
 * @return        {void}
 **/
void prepareSQL()
{
    columnValueV.resize(obtainCols.allColumnsInfo.size()); // 预先分配内存
    insertStatement.connect(&oracleConn);
    insertStatement.prepare(insertSQLStr);
    int colSeq = 1; // 输入变量参数
    // 准备更新表的SQL语句绑定变量
    for (int i = 0; i < obtainCols.allColumnsInfo.size(); i++)
    {
        // upttime和keyid这两个字段不需要绑定参数。
        if ((strcmp(obtainCols.allColumnsInfo[i].columnName, "update_time") == 0) ||
            (strcmp(obtainCols.allColumnsInfo[i].columnName, "key_id") == 0))
            continue;

        insertStatement.bindin(colSeq, columnValueV[i], obtainCols.allColumnsInfo[i].columnLen); // 绑定输入参数。
        // logFile.write("insertStatement.bindin(%d,columnValueV[%d],%d);\n", colSeq, i, obtainCols.allColumnsInfo[i].columnLen);

        colSeq++; // 输入参数的序号加1。
    }
    logFile.write("insert SQL prepare  successuflly.\n");
    if (xmlToDBArg.updateFlag != 1)
    {
        return;
    }
    updateStatement.connect(&oracleConn);
    updateStatement.prepare(updateSQLStr);
    colSeq = 1; // 输入变量参数
    // 绑定set部分的输入参数。
    for (int i = 0; i < obtainCols.allColumnsInfo.size(); i++)
    {
        if (obtainCols.allColumnsInfo[i].pkSeq != 0)
        {
            continue;
        }
        // upttime和keyid这两个字段不需要绑定参数。
        if ((strcmp(obtainCols.allColumnsInfo[i].columnName, "update_time") == 0) ||
            (strcmp(obtainCols.allColumnsInfo[i].columnName, "key_id") == 0))
            continue;

        updateStatement.bindin(colSeq, columnValueV[i], obtainCols.allColumnsInfo[i].columnLen); // 绑定输入参数。
        // logFile.write("updateStatement.bindin(%d,columnValueV[%d],%d);\n", colSeq, i, obtainCols.allColumnsInfo[i].columnLen);

        colSeq++; // 输入参数的序号加1。
    }

    // 绑定where部分参数
    for (int i = 0; i < obtainCols.allColumnsInfo.size(); i++) // 遍历全部字段的容器。
    {
        // 如果不是主键字段，跳过，只有主键字段才拼接在where的后面。
        if (obtainCols.allColumnsInfo[i].pkSeq == 0)
            continue;

        updateStatement.bindin(colSeq, columnValueV[i], obtainCols.allColumnsInfo[i].columnLen);
        // logfile.write("stmtupt.bindin(%d,vcolvalue[%d],%d);\n",colseq,ii,tcols.m_vallcols[ii].collen);
        // logFile.write("updateStatement.bindin(%d,columnValueV[%d],%d);\n", colSeq, i, obtainCols.allColumnsInfo[i].columnLen);
        colSeq++;
    }
    logFile.write("update SQL prepare  successuflly.\n");
    return;
}
/**
 * @brief        : 拼接插入和更新的SQL
 * @return        {void}
 **/
void jointSQL()
{
    string insertSQLField;  // insert语句的字段列表
    string insertSQLValues; // insert语句value后的内容
    int colSeq = 1;         // values 部分字段的序号
    for (auto &columnInfo : obtainCols.allColumnsInfo)
    {
        // update字段不处理
        if (strcmp(columnInfo.columnName, "update_time") == 0)
        {
            continue;
        }

        // 拼接insert语句的字段列表insertSQLField
        insertSQLField = insertSQLField + columnInfo.columnName + ",";

        // 拼接insert语句的的值insertSQLValues
        if (strcmp(columnInfo.columnName, "key_id") == 0)
        {
            insertSQLValues = insertSQLValues + sformat("SEQ_%s.nextval", xmlToDBArg.tableName + 2) + ",";
        }
        else
        {
            if (strcmp(columnInfo.dataType, "date") == 0) // 日期时间字段特殊处理
            {
                insertSQLValues = insertSQLValues + sformat("to_date(:%d,'yyyymmddhh24miss')", colSeq) + ',';
            }
            else
            {
                insertSQLValues = insertSQLValues + sformat(":%d", colSeq) + ",";
            }
            colSeq++; // key_id加1
        }
    }
    // 删除多余空格
    deleterchr(insertSQLField, ',');
    // 删除最后一个多余的逗号
    deleterchr(insertSQLValues, ',');
    // 拼接完整语句
    sformat(insertSQLStr, "insert into %s(%s) values(%s)", xmlToDBArg.tableName, insertSQLField.c_str(), insertSQLValues.c_str());
    logFile.write("insert SQL : %s\n", insertSQLStr.c_str());
    // 未指定拼接update语句
    if (xmlToDBArg.updateFlag != 1)
    {
        return;
    }
    updateSQLStr = sformat("update %s set ", xmlToDBArg.tableName);

    // 拼接update语句set后面的部分
    colSeq = 1;
    for (auto &columnInfo : obtainCols.allColumnsInfo)
    {
        if (columnInfo.pkSeq != 0)
        {
            continue;
        }
        // key_id不处理
        if (strcmp(columnInfo.columnName, "key_id") == 0)
        {
            continue;
        }
        // update_time字段直接复制sysdate
        if (strcmp(columnInfo.dataType, "update_time") == 0)
        {
            updateSQLStr = updateSQLStr + "update_time=sysdate";
            continue;
        }
        // 非date类型
        if (strcmp(columnInfo.dataType, "date") != 0)
        {
            updateSQLStr = updateSQLStr + sformat(" %s=:%d,", columnInfo.columnName, colSeq);
        }
        else
        {
            updateSQLStr = updateSQLStr + sformat(" %s=to_date(:%d,'yyyymmddhh24miss'), ", columnInfo.columnName, colSeq);
        }
        colSeq++;
    }
    deleterchr(updateSQLStr, ',');
    // 拼接where后面的内容
    updateSQLStr = updateSQLStr + " where 1=1";
    for (auto &columnInfo : obtainCols.allColumnsInfo) // 遍历表全部字段的容器。
    {
        if (columnInfo.pkSeq == 0)
            continue; // 如果不是主键字段，跳过。

        // 把主键字段拼接到update语句中，需要区分date字段和非date字段。
        if (strcmp(columnInfo.dataType, "date") != 0)
            updateSQLStr = updateSQLStr + sformat(" and %s=:%d", columnInfo.columnName, colSeq);
        else
            updateSQLStr = updateSQLStr + sformat(" and %s=to_date(:%d,'yyyymmddhh24miss')", columnInfo.columnName, colSeq);

        colSeq++; // 绑定变量的序号加1。
    }

    logFile.write("updateSQLStr=%s\n", updateSQLStr.c_str()); // 把更新表的SQL语句写日志，用于调试。

    return;
}
/**
 * @brief        : 把数据入库的参数配置文件runArg.inifilename加载到vxmltotable容器中。
 * @return        {bool} :成功返回true,失败返回false
 **/
bool loadXmlToTable()
{
    xmlToTableV.clear(); // 清空容器
    cifile inFile;
    if (inFile.open(runArg.iniFileName) == false) // 打开失败
    {
        logFile.write("file %s open failed, method:inFile.open(runArg.iniFileName).\n", runArg.iniFileName);
        return false;
    }
    string stringBuffer;
    while (true)
    {
        if (inFile.readline(stringBuffer, "<endl/>") == false) //
        {
            break;
        }
        getxmlbuffer(stringBuffer, "fileName", xmlToDBArg.fileName, 100);  // xml文件的匹配规则，用逗号分隔。
        getxmlbuffer(stringBuffer, "tableName", xmlToDBArg.tableName, 30); // 待入库的表名。
        getxmlbuffer(stringBuffer, "updateFlag", xmlToDBArg.updateFlag);   // 更新标志：1-更新；2-不更新。
        getxmlbuffer(stringBuffer, "execSQL", xmlToDBArg.execSQL, 300);    // 处理xml文件之前，执行的SQL语句。
        xmlToTableV.push_back(xmlToDBArg);
    }
    logFile.write("loadXmlToTable(%s) succeed.\n", runArg.iniFileName);
    return true;
}
/**
 * @brief        : 根据文件名从xmlToTableV容器中查找入库参数,存放在xmlToDBArg结构体中
 * @param         {string} &xmlFileName: 需要入库的文件
 * @return        {bool} :
 **/
bool findXmlToTable(const string &xmlFileName)
{
    for (auto &v : xmlToTableV)
    {
        if (matchstr(xmlFileName, v.fileName) == true)
        {
            xmlToDBArg = v;
            return true;
        }
    }

    return false;
}
/**
 * @brief        : 把xml解析到参数runArg结构中
 * @param         {char} *stringXMLBuffer: xml文件缓冲区
 * @return        {bool} : 成功返回true, 失败返回false
 **/
bool _xmlToRunArg(const char *stringXMLBuffer)
{
    memset(&runArg, 0, sizeof(struct runArg_t));

    getxmlbuffer(stringXMLBuffer, "connectStr", runArg.connectStr, 100);
    if (strlen(runArg.connectStr) == 0)
    {
        logFile.write("connectStr is null.\n");
        return false;
    }

    getxmlbuffer(stringXMLBuffer, "charset", runArg.charset, 50);
    if (strlen(runArg.charset) == 0)
    {
        logFile.write("charset is null.\n");
        return false;
    }

    getxmlbuffer(stringXMLBuffer, "iniFileName", runArg.iniFileName, 300);
    if (strlen(runArg.iniFileName) == 0)
    {
        logFile.write("inifilename is null.\n");
        return false;
    }

    getxmlbuffer(stringXMLBuffer, "xmlPath", runArg.xmlPath, 300);
    if (strlen(runArg.xmlPath) == 0)
    {
        logFile.write("xmlPath is null.\n");
        return false;
    }

    getxmlbuffer(stringXMLBuffer, "xmlPathBak", runArg.xmlPathBak, 300);
    if (strlen(runArg.xmlPathBak) == 0)
    {
        logFile.write("xmlPathBak is null.\n");
        return false;
    }

    getxmlbuffer(stringXMLBuffer, "xmlPathErr", runArg.xmlPathErr, 300);
    if (strlen(runArg.xmlPathErr) == 0)
    {
        logFile.write("xmlPathErr is null.\n");
        return false;
    }

    getxmlbuffer(stringXMLBuffer, "timeInterval", runArg.timeInterval);
    if (runArg.timeInterval < 2)
        runArg.timeInterval = 2;
    if (runArg.timeInterval > 30)
        runArg.timeInterval = 30;

    getxmlbuffer(stringXMLBuffer, "timeout", runArg.timeout);
    if (runArg.timeout == 0)
    {
        logFile.write("timeout is null.\n");
        return false;
    }

    getxmlbuffer(stringXMLBuffer, "procName", runArg.procName, 50);
    if (strlen(runArg.procName) == 0)
    {
        logFile.write("procName is null.\n");
        return false;
    }

    return true;
}
/**
 * @brief        : 数据入库业务函数
 * @return        {bool} :成功返回true,失败返回false
 **/
bool _xmlToDB()
{
    cdir dir;
    int intCnt = 50;
    while (true) // 每循环一次，执行一次入库任务。
    {
        // 把数据入库的参数配置文件starg.inifilename加载到vxmltotable容器中。
        if (intCnt > 30)
        {
            if (loadXmlToTable() == false)
                return false;
            intCnt = 0;
        }
        else
        {
            intCnt++;
        }

        // 打开runArg.xmlPath目录，为了保证先生成的xml文件先入库，打开目录的时候，应该按文件名排序。
        if (dir.opendir(runArg.xmlPath, "*.XML", 10000, false, true) == false)
        {
            logFile.write("file %s open failed ,method: dir.opendir(runArg.xmlPath, \" *.XML \", 10000, false, true) .\n", runArg.xmlPath);
            return false;
        }
        if (oracleConn.isopen() == false)
        {
            if (oracleConn.connecttodb(runArg.connectStr, runArg.charset) != 0)
            {
                logFile.write("connect database failed, connect string: %s.\n error message: %s\n", runArg.connectStr, oracleConn.message());
                return false;
            }
            logFile.write("connect database(%s) succeed.\n", runArg.connectStr);
        }
        while (true)
        {
            // 读取目录，得到一个xml文件。
            if (dir.readdir() == false)
                break;

            logFile.write("file:%s being processed...\n", dir.m_ffilename.c_str());
            // 处理xml文件的子函数，返回值：0-成功，其它的都是失败，失败的情况有很多种，暂时不确定。
            int ret = _xmlToDB(dir.m_ffilename, dir.m_filename);
            procAct.uptatime(); // 更新进程心跳
            // logFile << "succeed.\n";
            if (ret == 0) // 成功没有错误
            {
                logFile.write("file:%s processede succeed, total records = %d, insert records =%d,update records = %d, cost time = %.2f\n", xmlToDBArg.fileName, totalCnt, insertCnt, updateCnt, timer.elapsed());
                if (xmlToBak(dir.m_filename, runArg.xmlPath, runArg.xmlPathBak) == false)
                {
                    return false;
                }
            }
            // 1-入库参数不正确；3-待入库的表不存在；4-执行入库前的SQL语句失败。把xml文件移动到错误目录。
            if ((ret == 1) || (ret == 3) || (ret == 4))
            {
                if (ret == 1)
                    logFile << "failed,inputting database Arg error。\n";
                if (ret == 3)
                    logFile << "failed,inputting database table" << xmlToDBArg.tableName << ") unexistence\n";
                if (ret == 4)
                    logFile << "failed,execute SQL failed before inputting database.\n";

                // 把xml文件移动到starg.xmlpatherr参数指定的目录中，一般不会发生错误，如果真发生了，程序将退出。
                if (xmlToBak(dir.m_ffilename, runArg.xmlPath, runArg.xmlPathErr) == false)
                    return false;
            }

            // 2-数据库错误，函数返回，程序将退出。
            if (ret == 2)
            {
                logFile << "failed,database error.\n";
                return false;
            }

            // 5- 打开xml文件失败，函数返回，程序将退出。
            if (ret == 5)
            {
                logFile << "failed, file open failed.\n";
                return false;
            }
        }
        if (dir.size() == 0)
            sleep(runArg.timeInterval);
        procAct.uptatime(); // 更新进程心跳
    }

    return true;
}
/**
 * @brief        : 处理xml文件的子函数，返回值：0-成功，其它的都是失败，失败的情况有很多种，暂时不确定。
 * @param         {string} &fullFileName: 待入库的表的xml全路径文件名
 * @param         {string} &fileName:  xml文件名
 * @return        {int} : 0 -成功,其他-失败
 **/
int _xmlToDB(const string &fullFileName, const string &fileName)
{
    timer.start();
    // 1. 根据待入库的文件名,查找入库参数,得到对应表名
    if (findXmlToTable(fileName) == false)
    {
        return 1;
    }
    // 2. 根据表名,读取数据字典,得到字段名和主键
    if (obtainCols.getAllColumnsInfo(oracleConn, xmlToDBArg.tableName) == false)
    {
        return 2;
    }
    if (obtainCols.getPkColumnsInfo(oracleConn, xmlToDBArg.tableName) == false)
    {
        return 2;
    }
    // 3. 根据表的字段名和主键,拼接插入和更新表的SQL
    if (obtainCols.allColumnsInfo.size() == 0) // 表不存在
    {
        return 3;
    }
    // 拼接SQL语句
    jointSQL();
    // 准备SQL语句,绑定输入变量
    prepareSQL();
    if (executeSQL() == false) // 执行失败
    {
        return 4;
    }
    // 4. 打开xml文件
    cifile inFile;
    if (inFile.open(fullFileName) == false)
    {
        oracleConn.rollback();
        return 5;
    }
    string stringBuffer;
    while (true)
    {
        // 从xml文件读取一行数据
        if (inFile.readline(stringBuffer, "<endl/>") == false)
        {
            break;
        }
        totalCnt++;
        // 根据表的字段名,读取一行数据解析每个字段的值
        splitBuffer(stringBuffer);
        // 执行SQL语句
        if (insertStatement.execute() != 0) // 执行失败
        {
            if (insertStatement.rc() == 1) // 违反唯一性约束,表示记录已存在
            {
                if (xmlToDBArg.updateFlag == 1) // 执行更新
                {
                    if (updateStatement.execute() != 0) // 执行失败
                    {
                        logFile.write("update SQL: %s execute failed ,error message:%s\n", updateStatement.sql(), updateStatement.message());
                        logFile.write("readed content:%s\n", stringBuffer.c_str());
                    }
                    else
                    {
                        updateCnt++;
                    }
                }
                else // 插入失败其他原因
                {
                    logFile.write("insert SQL: %s execute failed ,error message:%s\n", updateStatement.sql(), updateStatement.message());
                    logFile.write("readed content:%s\n", stringBuffer.c_str());
                    if (insertStatement.rpc() == 3113 || insertStatement.rpc() == 3114 || insertStatement.rpc() == 3135 || updateStatement.rpc() == 16014)
                    {
                        // 如果是数据库系统出了问题，常见的问题如下，还可能有更多的错误，如果出现了，再加进来。
                        // ORA-03113: 通信通道的文件结尾；ORA-03114: 未连接到ORACLE；ORA-03135: 连接失去联系；ORA-16014：归档失败。
                        return 2;
                    }
                }
            }
            else
            {
            }
        }
        else
        {
            insertCnt++;
        }
        // 提交事务
        oracleConn.commit();
    }

    return 0;
}
/**
 * @brief        : 显示程序的帮助
 * @param         {char} *argv:
 * @return        {*}
 **/
void _help(char *argv[])
{
    printf("Using:/project/dataOpenPlatform/tools/bin/xmltodb logFilename xmlbuffer\n\n");

    printf("Sample:/project/dataOpenPlatform/tools/bin/procctl 10 /project/dataOpenPlatform/tools/bin/xmlToDB /log/idc/xmltodb_vip.log "
           "\"<connectStr>idc/idcpwd</connectStr>\n"
           "<charset>Simplified Chinese_China.AL32UTF8</charset>\n"
           "<iniFileName>/project/dataOpenPlatform/idc/ini/xmltodb.xml</iniFileName>\n"
           "<xmlPath>/idcdata/xmltodb/vip</xmlPath>\n"
           "<xmlPathBak>/idcdata/xmltodb/vipbak</xmlPathBak>\n"
           "<xmlPathErr>/idcdata/xmltodb/viperr</xmlPathErr>\n"
           "<timeInterval>5</timeInterval>\n"
           "<timeout>50</timeout><procName>xmltodb_vip</procName>\"\n\n");

    printf("本程序是共享平台的公共功能模块，用于把xml文件入库到Oracle的表中。\n");
    printf("logFilename   本程序运行的日志文件。\n");
    printf("xmlbuffer     本程序运行的参数，用xml表示，具体如下：\n\n");

    printf("connectStr     数据库的连接参数，格式：username/passwd@tnsname。\n");
    printf("charset     数据库的字符集，这个参数要与数据源数据库保持一致，否则会出现中文乱码的情况。\n");
    printf("iniFileName 数据入库的参数配置文件。\n");
    printf("xmlPath     待入库xml文件存放的目录。\n");
    printf("xmlPathBak  xml文件入库后的备份目录。\n");
    printf("xmlPathErr  入库失败的xml文件存放的目录。\n");
    printf("timeInterval     扫描xmlPath目录的时间间隔（执行入库任务的时间间隔），单位：秒，视业务需求而定，2-30之间。\n");
    printf("timeout     本程序的超时时间，单位：秒，视xml文件大小而定，建议设置30以上。\n");
    printf("procName       进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n");
}
/**
 * @brief        : 程序退出
 * @param         {int} sig: 信号
 * @return        {*}
 **/
void EXIT(int sig)
{
    logFile.write("程序退出，sig=%d\n\n", sig);

    oracleConn.disconnect();

    exit(0);
}