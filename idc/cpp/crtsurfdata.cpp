/**
 * @FilePath     : /dataOpenPlatform/idc/cpp/crtsurfdata.cpp
 * @Description  :  生成气象站点观测的分钟数据
 * @Author       : 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime : 2025-09-29 15:53:39
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "_public.h"
using namespace idc;

clogfile logFile; // 日志文件

void EXIT(int sig);

int main(int argc, char *argv[])
{
    // 站点参数文件  生成的测试数据存放的目录 本程序运行的日志 输出数据文件的格式
    if (argc != 4)
    {
        // 如果参数非法，给出帮助文档。
        cout << "Using:./crtsurfdata inifile outpath logfile \n";
        cout << "Examples:/project/dataOpenPlatform/idc/bin/crtsurfdata /project/dataOpenPlatform/idc/ini/stcode.ini /tmp/idc/surfdata /log/idc/crtsurfdata.log\n\n";

        cout << "本程序用于生成气象站点观测的分钟数据，程序每分钟运行一次，由调度模块启动。\n";
        cout << "inifile  气象站点参数文件名。\n";
        cout << "outpath  气象站点数据文件存放的目录。\n";
        cout << "logfile  本程序运行的日志文件名。\n";
        cout << "datafmt  输出数据文件的格式，支持csv、xml和json，中间用逗号分隔。\n\n";

        return -1;
    }
    closeioandsignal(true); // 关闭0、1、2和忽略全部的信号
    // 捕获到了2或15的信号
    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);
    // 打开失败
    if (!logFile.open(argv[3]))
    {
        cout << "logfile.open(" << argv[3] << ")failed.\n";
        return -1;
    }
    logFile.write("crtsurfdata 开始运行。\n");

    // 处理业务

    sleep(10);

    logFile.write("crtsurfdata 运行结束。\n");
    return 0;
}
/**
 * @brief        : 处理程序退出和信号2、15的处理函数
 * @param         {int} sig:
 * @return        {*}
**/
void EXIT(int sig)
{
    logFile.write("程序退出，sig=%d\n\n",sig);
    exit(0);
}