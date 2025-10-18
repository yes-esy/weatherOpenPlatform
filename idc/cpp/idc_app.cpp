/**
 * @FilePath     : /dataOpenPlatform/idc/cpp/idc_app.cpp
 * @Description  :  此程序是共享平台项目公用函数和类的定义文件。
 * @Author       : shengYang 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : shengYang 2900226123@qq.com
 * @LastEditTime : 2025-10-17 21:46:03
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "idc_app.h"
/**
 * @brief        : 把文件总读到的一行数据拆分到surfData_t结构体中;
 * @param         {string &} stringLine: 读取到的一行
 * @return        {bool} :返回true
 **/
bool CZHOBTMIND::splitBuffer(const string &stringLine, const bool isXml)
{
    memset(&mSurfRecordBind, 0, sizeof(surfData_t)); // 初始化
    if (isXml == true)                               // xml文件格式
    {
        getxmlbuffer(stringLine, "siteId", mSurfRecordBind.siteId, 5);
        getxmlbuffer(stringLine, "datetime", mSurfRecordBind.datetime, 14);
        char tmp[11];
        getxmlbuffer(stringLine, "t", tmp, 10);
        if (strlen(tmp) > 0)
        {
            snprintf(mSurfRecordBind.t, 10, "%d", (int)atof(tmp) * 10);
        }
        getxmlbuffer(stringLine, "p", tmp, 10);
        if (strlen(tmp) > 0)
        {
            snprintf(mSurfRecordBind.p, 10, "%d", (int)atof(tmp) * 10);
        }
        getxmlbuffer(stringLine, "u", mSurfRecordBind.u, 10);
        getxmlbuffer(stringLine, "wd", mSurfRecordBind.wd, 10);
        getxmlbuffer(stringLine, "wf", tmp, 10);
        if (strlen(tmp) > 0)
        {
            snprintf(mSurfRecordBind.wf, 10, "%d", (int)atof(tmp) * 10);
        }
        getxmlbuffer(stringLine, "r", tmp, 10);
        if (strlen(tmp) > 0)
        {
            snprintf(mSurfRecordBind.r, 10, "%d", (int)atof(tmp) * 10);
        }
        getxmlbuffer(stringLine, "vis", tmp, 10);
        if (strlen(tmp) > 0)
        {
            snprintf(mSurfRecordBind.vis, 10, "%d", (int)atof(tmp) * 10);
        }
    }
    else
    {
        ccmdstr cmdstr; // 用于拆分从文件中读取的行
        cmdstr.splittocmd(stringLine, ",");
        cmdstr.getvalue(0, mSurfRecordBind.siteId, 5);
        cmdstr.getvalue(1, mSurfRecordBind.datetime, 14);
        char tmp[11];
        cmdstr.getvalue(2, tmp, 10);
        if (strlen(tmp) > 0)
        {
            snprintf(mSurfRecordBind.t, 10, "%d", (int)atof(tmp) * 10);
        }
        cmdstr.getvalue(3, tmp, 10);
        if (strlen(tmp) > 0)
        {
            snprintf(mSurfRecordBind.p, 10, "%d", (int)atof(tmp) * 10);
        }
        cmdstr.getvalue(4, mSurfRecordBind.u, 10);
        cmdstr.getvalue(5, mSurfRecordBind.wd, 10);
        cmdstr.getvalue(6, tmp, 10);
        if (strlen(tmp) > 0)
        {
            snprintf(mSurfRecordBind.wf, 10, "%d", (int)atof(tmp) * 10);
        }
        cmdstr.getvalue(7, tmp, 10);
        if (strlen(tmp) > 0)
        {
            snprintf(mSurfRecordBind.r, 10, "%d", (int)atof(tmp) * 10);
        }
        cmdstr.getvalue(8, tmp, 10);
        if (strlen(tmp) > 0)
        {
            snprintf(mSurfRecordBind.vis, 10, "%d", (int)atof(tmp) * 10);
        }
    }
    mLineBuffer = stringLine;
    return true;
}
/**
 * @brief        : 把surData插入到ZHOBTMIND表中
 * @return        {bool} :执行成功返回true,失败返回false
 **/
bool CZHOBTMIND::insertTable()
{
    if (mStatement.isopen() == false)
    {
        // 准备操作表的sql语句,绑定输入参数。
        mStatement.connect(&mDatabaseCon);
        mStatement.prepare("insert into T_ZHOBTMIND(\"site_id\",\"visit_date\",\"t\",\"p\",\"u\",\"wd\",\"wf\",\"r\",\"vis\",\"key_id\")"
                           "values(:1,to_date(:2,'yyyymmddhh24miss'),:3,:4,:5,:6,:7,:8,:9,SEQ_ZHOBTMIND.nextval)");
        mStatement.bindin(1, mSurfRecordBind.siteId, 5);
        mStatement.bindin(2, mSurfRecordBind.datetime, 14);
        mStatement.bindin(3, mSurfRecordBind.t, 10);
        mStatement.bindin(4, mSurfRecordBind.p, 10);
        mStatement.bindin(5, mSurfRecordBind.u, 10);
        mStatement.bindin(6, mSurfRecordBind.wd, 10);
        mStatement.bindin(7, mSurfRecordBind.wf, 10);
        mStatement.bindin(8, mSurfRecordBind.r, 10);
        mStatement.bindin(9, mSurfRecordBind.vis, 10);
    }
    // 将数据插入表中
    if (mStatement.execute() != 0) // 执行失败
    {
        // 1.记录重复 2.数据内容非法
        if (mStatement.rc() != 1)
        {
            mLogFile.write("SQL execute failed, sql : %s ,method: mStatement.execute(), Error message : %s\n", mStatement.sql(), mStatement.message());
            mLogFile.write("stringLine=%s\n", mLineBuffer.c_str());
        }
        return false;
    }
    return true;
}