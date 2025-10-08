/**
 * @FilePath     : /dataOpenPlatform/tools/cpp/gzipFiles.cpp
 * @Description  :  压缩文件
 * @Author       : yes-esy 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : yes-esy 2900226123@qq.com
 * @LastEditTime : 2025-10-05 19:51:37
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
**/
#include "_public.h"
using namespace idc;
void EXIT(int sig);     // 程序退出处理函数
cpactive processActive; // 进程心跳
int main(int argc, char *argv[])
{
    // 程序的帮助
    if (argc != 4)
    {
        printf("\n");
        printf("Using:/project/dataOpenPlatform/tools/bin/gzipFiles pathname matchstr timeout\n\n");

        printf("Example:/project/dataOpenPlatform/tools/bin/gzipFiles /tmp/idc/surfdata \"*.xml,*.json\" 0.01\n");
        cout << R"(        /project/dataOpenPlatform/tools/bin/gzipFiles /log/idc "*.log.20*" 0.02)" << endl;
        printf("        /project/dataOpenPlatform/tools/bin/procctl 300 /project/tools/bin/gzipFiles /log/idc \"*.log.20*\" 0.02\n");
        printf("        /project/dataOpenPlatform/tools/bin/procctl 300 /project/tools/bin/gzipFiles /tmp/idc/surfdata \"*.xml,*.json\" 0.01\n\n");

        printf("这是一个工具程序，用于压缩历史的数据文件或日志文件。\n");
        printf("本程序把pathname目录及子目录中timeout天之前的匹配matchstr文件全部压缩，timeout可以是小数。\n");
        printf("本程序不写日志文件，也不会在控制台输出任何信息。\n");
        printf("本程序调用/usr/bin/gzip命令压缩文件。\n\n\n");

        return -1;
    }
    // 忽略全部的信号和关闭I/O,设置信号处理函数
    // closeioandsignal(true);
    signal(2, EXIT);
    signal(15, EXIT);

    processActive.addpinfo(120, "gzipFiles"); // 把当前进程添加到共享内存
    // 获取被定义为历史数据文件的时间点
    // string stringTimeout = ltime1("yyyymmddhh24miss", (int)(atof(argv[3]) * 24 * 60 * 60));
    string stringTimeout = ltime1("yyyymmddhh24miss", 0 - (int)(atof(argv[3]) * 24 * 60 * 60));
    // 打开目录
    cdir dir;
    if (!dir.opendir(argv[1], argv[2], 10000, true)) // 打开失败
    {
        printf("dir.open(%s) failed.\n", argv[1]);
        return -1;
    }
    // 遍历目录中的文件,如果是历史数据文件,压缩
    while (dir.readdir() == true)
    {
        if ((dir.m_mtime < stringTimeout) && matchstr(dir.m_ffilename, "*.gz") == false)
        {
            string strCmd = "/usr/bin/gzip -f " + dir.m_ffilename + " 1>/dev/null 2>/dev/null";
            if (system(strCmd.c_str()) == 0)
            {
                cout << "gzip" << dir.m_ffilename << " ok.\n";
            }
            else
            {
                cout << "gzip" << dir.m_ffilename << "failed.\n";
            }
        }
    }
    return 0;
}

/**
 * @brief        : 处理程序退出和信号2、15的处理函数
 * @param         {int} sig: 退出信号
 * @return        {void}
 **/
void EXIT(int sig)
{
    printf("程序退出，sig=%d\n\n", sig);
    exit(0);
}