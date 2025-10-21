/**
 * @FilePath     : /project/dataOpenPlatform/tools/cpp/_tools.h
 * @Description  :  工具头文件
 * @Author       : shengYang 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : shengYang 2900226123@qq.com
 * @LastEditTime : 2025-10-20 15:52:20
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/

#ifndef _TOOLS_H
#define _TOOLS_H

#include "_public.h"
#include "_ooci.h"

using namespace idc;
/**
 * 获取数据库表字段信息类
 */
class ObtainColumnsInfo
{
    struct columnInfo_t
    {
        char columnName[31]; // 列名
        char dataType[31];   // 列的数据类型,number,date,char
        int columnLen;       // 列的长度,number 22,date 9
        int pkSeq;           // 如果列是主键字段,存放主键字段的顺序
    };

public:
    ObtainColumnsInfo();                 // 显式声明构造函数
    vector<columnInfo_t> allColumnsInfo; // 全部字段的信息
    vector<columnInfo_t> pkColumnsInfo;  // 主键字段的信息
    string allColumnsName;               // 全部字段名列表,以字符串存放,用','隔开
    string pkColumnsNameStr;             // 主键字段列表,以字符串存放,用','隔开

    void initData();                                           // 成员变量初始化。
    bool getAllColumnsInfo(connection &conn, char *tableName); // 获取全部字段的信息
    bool getPkColumnsInfo(connection &conn, char *tableName);  // 获取主键字段的信息
};

#endif
