/**
 * @FilePath     : /dataOpenPlatform/idc/cpp/obtainCodeToDB.cpp
 * @Description  :  本程序用于将全国气象站点参数文件中的数据入库T_ZHOBTCODE表中
 * @Author       : shengYang 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : shengYang 2900226123@qq.com
 * @LastEditTime : 2025-10-16 16:41:03
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "_public.h"
#include "_ooci.h"
using namespace idc;
void EXIT(int sig);                           // 退出处理函数
clogfile logFile;                             // 日志文件
connection oracleCon;                         // 数据库连接
bool loadSitePosition(const string &inFile);  // 加载站点数据
list<struct sitePosition_t> sitePositionList; // 存放站点数据容器
cpactive procActive;                          // 进程心跳
struct sitePosition_t
{
    char provinceName[31]; // 省名
    char siteId[11];       // 站号
    char cityName[31];     // 站明
    char latitude[11];     // 纬度
    char longitude[11];    // 经度
    char height[11];       // 高度
} sitePos;
int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        printf("\n");
        printf("Using:./obtainCodeToDB inifile connstr charset logfile\n");

        printf("Example:/project/dataOpenPlatform/idc/bin/obtainCodeToDB /project/dataOpenPlatform/idc/ini/stcode.ini "
               "\"idc/idcpwd\" \"Simplified Chinese_China.AL32UTF8\" /log/idc/obtcodetodb.log\n\n");

        printf("本程序用于把全国气象站点参数数据保存到数据库的T_ZHOBTCODE表中，如果站点不存在则插入，站点已存在则更新。\n");
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
    procActive.addpinfo(10,"obtainCodeToDB");

    // 将数据加载到sitePositionList容器中
    if (loadSitePosition(argv[1]) == false)
    {
        EXIT(-1);
    }

    // 连接数据库
    if (oracleCon.connecttodb(argv[2], argv[3]) != 0)
    {
        logFile.write("connect database(%s) failed.\n%s\n", argv[2], oracleCon.message());
        EXIT(-1);
    }
    logFile.write("connect database(%s) succeed.\n", argv[2]);

    // 准备插入和更新表的sql语句
    sqlstatement statementIns(&oracleCon);
    statementIns.prepare("\
        insert into T_ZHOBTCODE(site_id,city_name,province_name,latitude,longitude,height,key_id) \
                                       values(:1,:2,:3,:4*100,:5*100,:6*10,SEQ_ZHOBTCODE.nextval)");
    statementIns.bindin(1, sitePos.siteId, 5);
    statementIns.bindin(2, sitePos.cityName, 30);
    statementIns.bindin(3, sitePos.provinceName, 30);
    statementIns.bindin(4, sitePos.latitude, 10);
    statementIns.bindin(5, sitePos.longitude, 10);
    statementIns.bindin(6, sitePos.height, 10);

    sqlstatement statementUpd(&oracleCon); // 更新表的sql语句。
    statementUpd.prepare("\
       update T_ZHOBTCODE set city_name=:1,province_name=:2,latitude=:3*100,longitude=:4*100,height=:5*10,update_time=sysdate \
         where site_id=:6");
    statementUpd.bindin(1, sitePos.cityName, 30);
    statementUpd.bindin(2, sitePos.provinceName, 30);
    statementUpd.bindin(3, sitePos.latitude, 10);
    statementUpd.bindin(4, sitePos.longitude, 10);
    statementUpd.bindin(5, sitePos.height, 10);
    statementUpd.bindin(6, sitePos.siteId, 5);
    int insertCnt = 0, updateCnt = 0; // 插入和更新的记录数
    ctimer timer;                     // 操作数据库消耗的时间
    for (auto &site : sitePositionList)
    {
        sitePos = site;
        if (statementIns.execute() != 0) // 插入执行失败
        {
            // 如果违反唯一约束,表示站点已存在,执行更新语句
            if (statementIns.rc() == 1)
            {
                // 执行更新语句
                if (statementUpd.execute() != 0) // 更新执行失败
                {
                    logFile.write("statementUpd.execute() failed.\n SQL is: %s \n Error Message: %s\n", statementUpd.sql(), statementUpd.message());
                    EXIT(-1);
                }
                else // 更新执行成功
                {
                    updateCnt++;
                }
            }
            else // 其他错误
            {
                logFile.write("statementIns.execute() failed.\n SQL is: %s \n Error Message: %s\n", statementIns.sql(), statementIns.message());
            }
        }
        else // 插入执行成功
        {
            insertCnt++;
        }
    }
    // 记录更新或者插入的记录数,以及消耗时长
    logFile.write("total records = %d, insert = %d, update = %d, time = %.2f s\n", sitePositionList.size(), insertCnt, updateCnt, timer.elapsed());

    oracleCon.commit();
    return 0;
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
/**
 * @brief        : 加载站点数据
 * @param         {string &} inFile: 输入文件
 * @return        {bool} : 加载成功返回true,失败返回false;
 **/
bool loadSitePosition(const string &inFile)
{
    cifile cInFile;            // 读取文件对象
    if (!cInFile.open(inFile)) // 打开失败
    {
        logFile.write("cInFile.open(%s)failed.\n"); // 写入日志
        return false;
    }
    string stringBuffer;            // 存放读取文件的每一行
    cInFile.readline(stringBuffer); // 读取站点第一行,表头,不处理

    ccmdstr cmdstr;                        // 用于拆分从文件中读取的行
    sitePosition_t sitePosition;           // 站点数据结构体
    while (cInFile.readline(stringBuffer)) // 一行一行读取
    {
        // logFile.write("stringBuffer=%s\n", stringBuffer.c_str());
        cmdstr.splittocmd(stringBuffer, ",");             // 拆分字符串
        memset(&sitePosition, 0, sizeof(sitePosition_t)); // 清空结构体

        cmdstr.getvalue(0, sitePosition.provinceName, 30); // 省
        cmdstr.getvalue(1, sitePosition.siteId, 5);        // 站点代码
        cmdstr.getvalue(2, sitePosition.cityName, 30);     // 站名
        cmdstr.getvalue(3, sitePosition.latitude, 10);     // 纬度
        cmdstr.getvalue(4, sitePosition.longitude, 10);    // 经度
        cmdstr.getvalue(5, sitePosition.height, 10);       // 高度
        sitePositionList.push_back(sitePosition);          // 放入容器
    }
    // 不需要手动关闭文件, 析构函数已经实现

    // 把容器中的全部数据写入日志
    // for (const auto &site : siteList)
    // {
    //     logFile.write("provinceName=%s,siteId=%s,cityName=%s,latitude=%.2f,longtitude=%.2f,height=%.2f\n",\
    //     site.provinceName,site.siteId,site.cityName,site.latitude,site.longtitude,site.height);
    // }

    return true;
}