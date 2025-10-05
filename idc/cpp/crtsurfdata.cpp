/**
 * @FilePath     : /dataOpenPlatform/idc/cpp/crtsurfdata.cpp
 * @Description  :  生成气象站点观测的分钟数据
 * @Author       : 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : yes-esy 2900226123@qq.com
 * @LastEditTime : 2025-10-05 15:12:58
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "_public.h"
using namespace idc;
clogfile logFile; // 日志文件
/**
 * 站点
 * 省 站号 站名 纬度 经度 海拔高度
 */
struct sitePosition_t
{
    char provinceName[31]; // 省名
    char siteId[11];       // 站号
    char siteName[31];     // 站明
    double latitude;       // 纬度；
    double longtitude;     // 经度
    double height;         // 高度
};
/**
 * 观测数据
 */
struct surfData_t
{
    char siteId[11];   // 站点代码
    char datetime[15]; // 数据时间: 格式yyyymmddhh24miss,精确到分钟，秒固定天00.
    int t;             // 气温: 单位，0.1摄氏度
    int p;             // 气压: 单位，0.1百帕
    int u;             // 相对湿度:0-100之间的值
    int wd;            // 风向:0~360之间的值
    int wf;            // 风速:单位0.1m/s
    int r;             // 降雨量:0.1mm
    int vis;           // 能见度
};
list<struct sitePosition_t> siteList;                             // 存放所有站点数据
list<struct surfData_t> visDatalist;                              // 观测数据列表容器
char strDatetime[15];                                             // 系统时间
void EXIT(int sig);                                               // 退出
bool loadSitePosition(const string &inFile);                      // 加载站点数据
void generateVisdata();                                           // 生成观测数据存放在visDataList中
bool writeSurfFile(const string &outpath, const string &datafmt); // 将数据写入文件
cpactive procActive;                                              // 全局进程心跳对象
int main(int argc, char *argv[])
{
    // 站点参数文件  生成的测试数据存放的目录 本程序运行的日志 输出数据文件的格式
    if (argc != 5)
    {
        // 如果参数非法，给出帮助文档。

        cout << "Using:./crtsurfdata inifile outpath logfile datafmt\n";
        cout << "Examples:/project/dataOpenPlatform/tools/bin/procctl 60 /project/dataOpenPlatform/idc/bin/crtsurfdata /project/dataOpenPlatform/idc/ini/stcode.ini /tmp/idc/surfdata /log/idc/crtsurfdata.log csv,xml,json\n\n";

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

    procActive.addpinfo(10, "crtsurfdata"); // 把当前进程放入共享内存
    // 打开失败
    if (!logFile.open(argv[3]))
    {
        cout << "logfile.open(" << argv[3] << ")failed.\n";
        return -1;
    }
    logFile.write("crtsurfdata 开始运行。\n");

    // 加载站点数据
    if (!loadSitePosition(argv[1]))
    {
        EXIT(-1);
    }
    // 获取数据时间
    memset(strDatetime, 0, sizeof(strDatetime));
    ltime(strDatetime, "yyyymmddhh24miss"); // 获取系统当前时间
    strncpy(strDatetime + 12, "00", 2);     // 把数据时间中的秒固定填00

    // 生成观测数据；
    generateVisdata();

    if (strstr(argv[4], "csv"))
    {
        writeSurfFile(argv[2], "csv");
    }
    if (strstr(argv[4], "xml"))
    {
        writeSurfFile(argv[2], "xml");
    }
    if (strstr(argv[4], "json"))
    {
        writeSurfFile(argv[2], "json");
    }

    logFile.write("crtsurfdata 运行结束。\n");
    return 0;
}
/**
 * @brief        : 处理程序退出和信号2、15的处理函数
 * @param         {int} sig: 退出信号
 * @return        {void}
 **/
void EXIT(int sig)
{
    logFile.write("程序退出，sig=%d\n\n", sig);
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
        cmdstr.getvalue(1, sitePosition.siteId, 10);       // 站点代码
        cmdstr.getvalue(2, sitePosition.siteName, 30);     // 站名
        cmdstr.getvalue(3, sitePosition.latitude);         // 纬度
        cmdstr.getvalue(4, sitePosition.longtitude);       // 经度
        cmdstr.getvalue(5, sitePosition.height);           // 高度
        siteList.push_back(sitePosition);                  // 放入容器
    }
    // 不需要手动关闭文件, 析构函数已经实现

    // 把容器中的全部数据写入日志
    // for (const auto &site : siteList)
    // {
    //     logFile.write("provinceName=%s,siteId=%s,siteName=%s,latitude=%.2f,longtitude=%.2f,height=%.2f\n",\
    //     site.provinceName,site.siteId,site.siteName,site.latitude,site.longtitude,site.height);
    // }

    return true;
}
/**
 * @brief        : 模拟生成每分钟观测数据存放在visDataList中
 * @return        {void}
 **/
void generateVisdata()
{
    srand(time(0)); // 随机数种子

    surfData_t surfData; // 观测数据结构体；

    for (const auto &site : siteList) // 为每个站点生成观测数据
    {
        memset(&surfData, 0, sizeof(surfData_t)); // 初始化

        strcpy(surfData.siteId, site.siteId);   // 站点代码
        strcpy(surfData.datetime, strDatetime); // 填充时间
        surfData.t = rand() % 350;              // 气温
        surfData.p = rand() % 265 + 10000;      // 气压
        surfData.u = rand() % 101;              // 相对湿度
        surfData.wd = rand() % 360;             // 风向
        surfData.wf = rand() % 150;             // 风速
        surfData.r = rand() % 16;               // 降雨
        surfData.vis = rand() % 5001 + 100000;  // 能见度
        visDatalist.push_back(surfData);        // 放入容器
    }

    // 写入日志文件
    // for (const auto &data : visDatalist)
    // {
    //     logFile.write("siteId=%s,datetime=%s,t=%.1f,p=%.1f,u=%d,wd=%d,wf=%.1f,r=%.1f,vis=%.1f\n",
    //                   data.siteId, data.datetime, data.t / 10.0, data.p / 10.0, data.u, data.wd, data.wf / 10.0, data.r / 10.0, data.vis / 10.0);
    // }
}
/**
 * @brief        : 将数据写入文件
 * @param         {string} &outpath: 写入文件的路径
 * @param         {string} &datafmt: 写入数据的格式csv,xml,json等
 * @return        {bool} : 成功返回true,失败返回false
 **/
bool writeSurfFile(const string &outpath, const string &datafmt)
{
    // 拼接文件名, eg: path/perfix_time_pid.fmt
    string fileName = outpath + "/" + "SURF_ZH_" + strDatetime + "_" + to_string(getpid()) + "." + datafmt;
    cofile outFile; // 写入数据文件对象

    if (!outFile.open(fileName)) // 打开失败
    {
        logFile.write("outFile.open(%s) failed.\n", fileName.c_str());
        return false;
    }

    // 把dataList容器中的观测数据写入文件
    if (datafmt == "csv")
    {
        outFile.writeline("站点代码,数据时间,气温,气压,相对湿度,风向,风速,降水量,能见度\n"); // 写入表头
    }
    if (datafmt == "xml")
    {
        outFile.writeline("<data>\n");
    }
    if (datafmt == "json")
    {
        outFile.writeline("{\"data\":[\n");
    }
    int idx = 0;
    int total = visDatalist.size();
    for (const auto &data : visDatalist)
    {
        if (datafmt == "csv")
        {
            outFile.writeline("%s,%s,%.1f,%.1f,%d,%d,%.1f,%.1f,%.1f\n",
                              data.siteId, data.datetime, data.t / 10.0, data.p / 10.0, data.u, data.wd, data.wf / 10.0, data.r / 10.0, data.vis / 10.0);
        }
        if (datafmt == "xml")
        {
            outFile.writeline("<siteId>%s</siteId><datetime>%s</datetime><t>%.1f</t><p>%.1f</p><u>%d</u>"
                              "<wd>%d</wd><wf>%.1f</wf><r>%.1f</r><vis>%.1f</vis><endl/>\n",
                              data.siteId, data.datetime, data.t / 10.0, data.p / 10.0, data.u, data.wd, data.wf / 10.0, data.r / 10.0, data.vis / 10.0);
        }
        if (datafmt == "json")
        {
            outFile.writeline("{\"siteId\":\"%s\",\"datetime\":\"%s\",\"t\":\"%.1f\",\"p\":\"%.1f\",\"u\":\"%d\",\"wd\":\"%d\",\"wf\":\"%.1f\",\"r\":\"%.1f\",\"vis\":\"%.1f\"}",
                              data.siteId, data.datetime, data.t / 10.0, data.p / 10.0, data.u, data.wd, data.wf / 10.0, data.r / 10.0, data.vis / 10.0);
            idx++;
            if (idx < total)
                outFile.writeline(",\n");
            else
                outFile.writeline("\n");
        }
    }
    if (datafmt == "xml")
    {
        outFile.writeline("</data>\n");
    }
    if (datafmt == "json")
    {
        outFile.writeline("]}\n");
    }
    outFile.closeandrename(); // 关闭临时文件,并改名正式文件
    logFile.write("生成数据文件%s成功,数据时间%s,记录数%d。\n", fileName.c_str(), strDatetime, visDatalist.size());
    return true;
}