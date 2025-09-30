/**
 * @FilePath     : /dataOpenPlatform/idc/cpp/crtsurfdata.cpp
 * @Description  :  生成气象站点观测的分钟数据
 * @Author       : 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : yes-esy 2900226123@qq.com
 * @LastEditTime : 2025-09-30 20:10:48
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
list<struct sitePosition_t> siteList;        // 存放所有站点数据
list<struct surfData_t> visDatalist;         // 观测数据列表容器
char strDatetime[15];                        // 系统时间
void EXIT(int sig);                          // 退出
bool loadSitePosition(const string &inFile); // 加载站点数据
void generateVisdata();                      // 生成观测数据存放在visDataList中
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

    sleep(10);

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
    for (const auto &data : visDatalist)
    {
        logFile.write("siteId=%s,datetime=%s,t=%d,p=%d,u=%d,wd=%d,wf=%d,r=%d,vis=%d\n",
                      data.siteId, data.datetime, data.t, data.p, data.u, data.wd, data.wf, data.r, data.vis);
    }
}