/**
 * @FilePath     : /project/dataOpenPlatform/tools/cpp/_tools.cpp
 * @Description  :  
 * @Author       : shengYang 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : shengYang 2900226123@qq.com
 * @LastEditTime : 2025-10-20 15:50:12
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
**/
#include"_tools.h"
// 获取表全部的列和主键列信息的类。
ObtainColumnsInfo::ObtainColumnsInfo()
{
    initData();// 调用成员变量初始化函数。
}

void ObtainColumnsInfo::initData() // 成员变量初始化。
{
    allColumnsInfo.clear();
    pkColumnsInfo.clear();
    allColumnsName.clear();
    pkColumnsNameStr.clear();
}
/**
 * @brief        : 获取全部字段的信息
 * @param         {connection} &conn: 数据库连接对象
 * @param         {char} *tableName: 表名
 * @return        {bool} : 成功返回true,失败返回false
 **/
bool ObtainColumnsInfo:: getAllColumnsInfo(connection &conn, char *tableName)
{
    allColumnsInfo.clear(); // 清空全部字段信息容器
    allColumnsName.clear(); // 清空全部字段名字符串
    columnInfo_t columnInfo;
    sqlstatement statement;
    statement.connect(&conn);
    statement.prepare("\
        select lower(column_name),lower(data_type),data_length from USER_TAB_COLUMNS\
        where table_name=upper(:1) order by column_id");
    statement.bindin(1, tableName, 30);
    statement.bindout(1, columnInfo.columnName);
    statement.bindout(2, columnInfo.dataType);
    statement.bindout(3, columnInfo.columnLen);

    if (statement.execute() != 0)
    {
        // logFile.write("SQL:%s execute failedn\n,error message:%s\n", statement.sql(), statement.message());
        return false;
    }
    while (true) // 处理找到的数据
    {
        memset(&columnInfo, 0, sizeof(columnInfo_t));
        if (statement.next() != 0)
        {
            break;
        }
        // 列的数据类型分为char,date,number三类
        if (strcmp(columnInfo.dataType, "char") == 0)
        {
            strcpy(columnInfo.dataType, "char");
        }
        if (strcmp(columnInfo.dataType, "nchar") == 0)
        {
            strcpy(columnInfo.dataType, "char");
        }
        if (strcmp(columnInfo.dataType, "varchar2") == 0)
        {
            strcpy(columnInfo.dataType, "char");
        }
        if (strcmp(columnInfo.dataType, "nvarchar2") == 0)
        {
            strcpy(columnInfo.dataType, "char");
        }
        if (strcmp(columnInfo.dataType, "row_id") == 0)
        {
            strcpy(columnInfo.dataType, "char");
            columnInfo.columnLen = 18;
        }

        if (strcmp(columnInfo.dataType, "date") == 0)
        {
            strcpy(columnInfo.dataType, "date");
            columnInfo.columnLen = 14;
        }

        if (strcmp(columnInfo.dataType, "number") == 0)
        {
            strcpy(columnInfo.dataType, "number");
        }
        if (strcmp(columnInfo.dataType, "integer") == 0)
        {
            strcpy(columnInfo.dataType, "number");
        }
        if (strcmp(columnInfo.dataType, "float") == 0)
        {
            strcpy(columnInfo.dataType, "number");
        }
        // 如果非三种基本类型忽略
        if (((strcmp(columnInfo.dataType, "char") != 0)) &&
            (strcmp(columnInfo.dataType, "date") != 0) &&
            (strcmp(columnInfo.dataType, "number") != 0))
        {
            continue;
        }
        // number长度为22
        if (strcmp(columnInfo.dataType, "number") == 0)
        {
            columnInfo.columnLen = 22;
        }
        allColumnsName = allColumnsName + columnInfo.columnName + ",";
        allColumnsInfo.push_back(columnInfo);
    }
    if (statement.rpc() > 0)
    {
        deleterchr(allColumnsName, ',');
    }
    return true;
}
/**
 * @brief        : 获取主键字段的信息
 * @param         {connection} &conn: 数据库连接对象
 * @param         {char} *tableName: 表名
 * @return        {bool} : 成功返回true,失败返回false
 **/
bool ObtainColumnsInfo::getPkColumnsInfo(connection &conn, char *tableName)
{
    pkColumnsInfo.clear();
    pkColumnsNameStr.clear();
    columnInfo_t columnInfo;
    sqlstatement statement;
    statement.connect(&conn);
    statement.prepare(
        "select lower(column_name),position \
                from USER_CONS_COLUMNS \
                where table_name=upper(:1) \
                and constraint_name = (select constraint_name \
                                             from USER_CONSTRAINTS\
                                             where table_name=upper(:2) \
                                                    and constraint_type='P' \
                                                    and generated='USER NAME')\
                order by position");
    statement.bindin(1, tableName, 30);
    statement.bindin(2, tableName, 30);
    statement.bindout(1, columnInfo.columnName, 30);
    statement.bindout(2, columnInfo.pkSeq);
    if (statement.execute() != 0)
    {
        return false;
    }
    while (true)
    {
        memset(&columnInfo, 0, sizeof(columnInfo_t));
        if (statement.next() != 0)
        {
            break;
        }
        pkColumnsNameStr = pkColumnsNameStr + ",";
        pkColumnsInfo.push_back(columnInfo);
    }
    if (statement.rpc() > 0)
    {
        deleterchr(pkColumnsNameStr, ',');
    }
    for (auto &pkInfo : pkColumnsInfo)
    {
        for (auto &commonInfo : allColumnsInfo)
        {
            if (strcmp(pkInfo.columnName, commonInfo.columnName) == 0)
            {
                commonInfo.pkSeq = pkInfo.pkSeq;
                break;
            }
        }
    }
    return true;
}
