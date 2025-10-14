/**
 * @FilePath     : /dataOpenPlatform/tools/cpp/ftpPutFiles.cpp
 * @Description  :  文件上传模块
 * @Author       : yes-esy 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : yes-esy 2900226123@qq.com
 * @LastEditTime : 2025-10-11 19:46:32
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/

#include "_public.h"
#include "_ftp.h"

using namespace idc;
/**
 * 程序运行参数
 */
struct arg_t
{
    char host[31];            // ftp服务端的ip和端口
    int modes;                // 传输模式: 1.passive被动 2.active 主动
    char username[31];        // 登录用户名
    char password[31];        // 登录密码
    char remotePath[256];     // 远程服务器存放文件的目录
    char localPath[256];      // 本地存放文件的目录
    char matchName[256];      // 待下载文件的匹配规则
    int pType;                // 下载后服务器端的处理方式 : 1.不处理 2. 删除本地文件 3. 备份
    char localPathBak[256];   // 远程备份目录
    char uploadFileName[256]; // 已下载成功文件名清单
    int timeout;              // 进程心跳超时的时间。
    char procName[51];        // 进程名
};
arg_t arg; // 程序运行参数
/**
 * 文件信息结构体
 */
struct fileInfo_t
{
    string fileName;                                                                                            // 文件名
    string editTime;                                                                                            // 修改列表
    fileInfo_t() = default;                                                                                     // 默认构造函数
    fileInfo_t(const string &InfileName, const string &InEditTime) : fileName(InfileName), editTime(InEditTime) // 含参构造函数
    {
    }
    /**
     * 清空文件信息
     */
    void clear()
    {
        fileName.clear();
        editTime.clear();
    }
};
cpactive procAct;                                    // 进程心跳
map<string, string> uploadedFiles;                   // 容器一:存放已经成功上传的文件,uploadFileName中加载
list<fileInfo_t> nlistFiles;                         // 容器二:下载前列出客户端文件名的容器,从nlist文件中加载
list<fileInfo_t> skippedFiles;                       // 容器三:本次不需要上传的文件
list<fileInfo_t> filesToUpload;                      // 容器四:本次需要下载的文件
void EXIT(int sig);                                  // 程序退出处理函数
cftpclient ftp;                                      // 创建ftp客户端对象
void _help();                                        // 帮助文档·
clogfile logFile;                                    // 全局日志文件
bool xmlLoadArg(const char *xmlFile);                // 解析xml文件
bool loadLocalFileList();                            // 加载文件
bool loadUpoadedFiles();                             // 加载已经上传好的文件名
void cmpContainer();                                 // 比较不同容器的文件信息更新skippedFiles和filesToUpload
void writeSkippedFiles();                            // 把容器skippedFiles中的数据写入arg,uploadFileName文件,覆盖之前的
void appendSkippedFiles(const fileInfo_t &fileInfo); // 把上传成功的文件记录追加到arg.uploadFileName文件中。
int main(int argc, char *argv[])
{
    // 第一步计划:从服务器某个目录中下载文件,可以指定文件名匹配的规则
    // main()的参数: 日志文件名 ftp服务端的ip和端口 ftp的传输模式 ftp用户名 ftp密码 服务端的目录名 本地目录名 文件名匹配的规则
    if (argc != 3)
    {
        _help();
        return -1;
    }
    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);

    // 打开日志文件
    if (logFile.open(argv[1]) == false)
    {
        logFile.write("打开日志文件失败 logFile.open(%s),\n", argv[1]);
        return -1;
    }
    // 解析xml,得到程序的运行参数
    if (xmlLoadArg(argv[2]) == false)
    {
        logFile.write("解析xml文件失败 xmlLoadArg(%s),\n", argv[1]);
        return -1;
    }
    procAct.addpinfo(arg.timeout, arg.procName);
    // 登录ftp服务器
    if (ftp.login(arg.host, arg.username, arg.password, arg.modes) == false) // 登录失败
    {
        logFile.write("ftp login failed: %s\n", ftp.response());
        return -1;
    }
    logFile.write("ftp login success.\n");
    // 进入ftp服务器存放文件的目录
    if (ftp.chdir(arg.remotePath) == false) // 进入目录失败
    {
        logFile.write("enter remote path failed, ftp.chdir(%s) failed: %s\n", arg.remotePath, ftp.response());
        return -1;
    }
    // 调用ftpclient.nlist()方法列出目录中的文件名,保存载本地文件夹中
    if (ftp.nlist(".", sformat("/tmp/nlist/ftpGetFiles_%d.nlist", getpid())) == false)
    {
        logFile.write("list directory failed, ftp.nlist(%s) failed\n", arg.remotePath);
        return -1;
    }
    logFile.write("nlist(%s) ok.\n", sformat("/tmp/nlist/ftpGetFiles_%d.nlist", getpid()).c_str());
    procAct.uptatime(); // 更新进程状态
    // 把ftpclient.nlist()方法获取到的list文件加载到容器nlistFiles中
    if (loadLocalFileList() == false)
    {
        logFile.write("loadFileList() failed: %s.\n", ftp.response());
        return -1;
    }

    if (arg.pType == 1)
    {
        // 加载已下载的文件信息
        loadUpoadedFiles();
        // 比较nlistFiles和uploadedFiles的文件信息,更新skippedFiles和filesToUpload
        cmpContainer();
        // 把容器skippedFiles中的数据写入arg,uploadFileName文件,覆盖之前的
        writeSkippedFiles();
    }
    else
    {
        nlistFiles.swap(filesToUpload);
    }
    procAct.uptatime(); // 更新进程状态
    // 遍历nlistFiles容器
    string reFileName;  // 远程文件路径
    string locFileName; // 本地文件路径
    for (const auto &file : nlistFiles)
    {
        sformat(reFileName, "%s/%s", arg.remotePath, file.fileName.c_str()); // 拼接服务端全路径文件名
        sformat(locFileName, "%s/%s", arg.localPath, file.fileName.c_str()); // 拼接本地全路径文件名
        logFile.write("get %s ... ", reFileName.c_str());
        // 调用get方法下载文件
        if (ftp.get(reFileName, locFileName) == false) // 下载失败
        {
            logFile.write("ftp.get(%s,%s) failed: %s.\n", reFileName.c_str(), locFileName.c_str(), ftp.response());
            return -1;
        }
        logFile.write("file(%s) proccess succeed.\n", file.fileName.c_str());
        procAct.uptatime(); // 更新进程状态
        // 下载成功后服务器的处理
        if (arg.pType == 1) // 增量下载文件
        {
            appendSkippedFiles(file);
        }
        else if (arg.pType == 2) // 删除服务器文件
        {
            if (ftp.ftpdelete(reFileName) == false) // 删除失败
            {
                logFile.write("remote file is %s,ftp ftpdelete failed:%s. \n", reFileName, ftp.response());
                return -1;
            }
        }
        else if (arg.pType == 3) // 将文件移动到备份目录
        {
            string remoteFileBakPath = sformat("%s/%s", arg.localPathBak, file.fileName.c_str()); // 远程备份目录
            if (ftp.ftprename(reFileName, remoteFileBakPath) == false)                            // 移动失败
            {
                logFile.write("ftp ftprename(%s,%s),failed:%s\n", reFileName.c_str(), remoteFileBakPath.c_str(), ftp.response());
            }
        }
    }

    return 0;
}
/**
 * @brief        : 程序帮助文档
 * @return        {*}
 **/
void _help()
{
    printf("\n");
    printf("Using:/project/dataOpenPlatform/tools/bin/ftpGetFiles logfilename xmlbuffer\n\n");

    //printf("Sample:/peoject/dataOpenPlatform/tools/bin/procctl 30 /peoject/dataOpenPlatform/tools/bin/ftpGetFiles /log/idc/ftpGetFiles_surfdata.log " \
    //          "\"<host>192.168.150.128:21</host><mode>1</mode>"\
    //          "<username>wucz</username><password>oracle</password>"\
    //          "<remotepath>/tmp/idc/surfdata</remotepath><localpath>/idcdata/surfdata</localpath>"\
    //          "<matchname>SURF_ZH*.XML,SURF_ZH*.CSV</matchname>"\
    //          "<ptype>3</ptype><localPathBak>/tmp/idc/surfdatabak</localPathBak>\"\n\n");
    printf("Sample:\n\
/project/tools/bin/procctl 30 /project/dataOpenPlatform/tools/bin/ftpPutFiles /log/idc/ftpPutFiles_test.log \"<host>111.228.47.8:21</host>\n\
<mode>1</mode>\n\
<username>yes</username>\n\
<password>123456789yes</password>\n\
<remotePath>/srv/FTPServer</remotePath>\n\
<localPath>/tmp/idc/ftp/client/download/surfdata</localPath>\n\
<matchName>SURF_ZH*.XML,SURF_ZH*.CSV,SURF_ZH*.JSON</matchName>\n\
<pType>1</pType>\n\
<localPathBak>/srv/FTPServer/bak</localPathBak>\n\
<uploadFileName>/tmp/idc/ftp/client/ftpGetFiles.xml</uploadFileName>\n\
<timeout>30</timeout>\n\
<procName>ftpgetfiles_test</procName>\n\n\n");

    printf("本程序是通用的功能模块，用于把远程ftp服务端的文件下载到本地目录。\n");
    printf("logfilename是本程序运行的日志文件。\n");
    printf("xmlbuffer为文件下载的参数，如下：\n");
    printf("<host>111.228.47.8:21</host> 远程服务端的IP和端口。\n");
    printf("<mode>1</mode> 传输模式，1-被动模式，2-主动模式，缺省采用被动模式。\n");
    printf("<username>yes</username> 远程服务端ftp的用户名。\n");
    printf("<password>123456789yes.</password> 远程服务端ftp的密码。\n");
    printf("<remotePath>/tmp/idc/download/surfdata</remotePath> 远程服务端存放文件的目录。\n");
    printf("<localPath>/idcdata/surfdata</localPath> 本地文件存放的目录。\n");
    printf("<matchName>SURF_ZH*.XML,SURF_ZH*.CSV</matchName> 待下载文件匹配的规则。"
           "不匹配的文件不会被下载，本字段尽可能设置精确，不建议用*匹配全部的文件。\n");
    printf("<pType>1</pType> 文件下载成功后远程服务端的处理方式: 1.什么也不做; 2.删除 3.备份;如果为3还要指定备份的目录\n");
    printf("<localPathBak></localPathBak> 文件下载成功后,服务端文件的备份目录, 此参数只有当ptype为3时才生效。\n");
    printf("<uploadFileName></uploadFileName> 已下载成功文件名清单。此参数只有pType=1时生效\n\n\n");
}
/**
 * @brief        : 加载xml文件
 * @param         {char} *xmlFile: xml文件
 * @return        {bool} : 成功返回true , 失败返回false
 **/
bool xmlLoadArg(const char *xmlFile)
{
    memset(&arg, 0, sizeof(arg_t)); // 初始化参数列表

    getxmlbuffer(xmlFile, "host", arg.host, 30); // 远程服务端ftp的用户名

    if (strlen(arg.host) == 0) // 用户名为空
    {
        logFile.write("ftp host is null.\n");
        return false;
    }

    getxmlbuffer(xmlFile, "mode", arg.modes); // 设置传输模式

    if (arg.modes != 2) // 不为2,强制为1
    {
        arg.modes = 1;
    }

    getxmlbuffer(xmlFile, "username", arg.username, 30); // 远程服务端ftp的用户名

    if (strlen(arg.username) == 0) // 用户名为空
    {
        logFile.write("ftp username is null.\n");
        return false;
    }

    getxmlbuffer(xmlFile, "password", arg.password, 30); // 获取密码
    if (strlen(arg.password) == 0)                       // 用户名为空
    {
        logFile.write("ftp password is null.\n");
        return false;
    }

    getxmlbuffer(xmlFile, "remotePath", arg.remotePath, 255); // 获取服务端存放文件目录
    if (strlen(arg.remotePath) == 0)                          // 服务端存放文件目录为空
    {
        logFile.write("ftp remotePath is null.\n");
        return false;
    }

    getxmlbuffer(xmlFile, "localPath", arg.localPath, 255); // 获取本地存放文件目录
    if (strlen(arg.localPath) == 0)                         // 本地放文件目录为空
    {
        logFile.write("ftp localPath is null.\n");
        return false;
    }

    getxmlbuffer(xmlFile, "matchName", arg.matchName, 100); // 获取待下载文件匹配的规则
    if (strlen(arg.matchName) == 0)                         // 待下载文件匹配的规则为空
    {
        logFile.write("ftp matchName is null.\n");
        return false;
    }
    getxmlbuffer(xmlFile, "pType", arg.pType); // 获取下载文件后服务器的下载方式
    if (arg.pType == 1)
    {
        getxmlbuffer(xmlFile, "uploadFileName", arg.uploadFileName);
        if (strlen(arg.uploadFileName) == 0)
        {
            logFile.write("uploadFileName bak is null\n");
            return false;
        }
    }
    else if (arg.pType == 2)
    {
    }
    else if (arg.pType == 3)
    {
        getxmlbuffer(xmlFile, "localPathBak", arg.localPathBak);
        if (strlen(arg.localPathBak) == 0)
        {
            logFile.write("local path bak is null\n");
            return false;
        }
    }
    else
    {
        logFile.write("load arg.pType error pType = %d\n", arg.pType);
        return false;
    }
    getxmlbuffer(xmlFile, "timeout", arg.timeout); // 进程心跳的超时时间。
    if (arg.timeout == 0)
    {
        logFile.write("timeout is null.\n");
        return false;
    }
    getxmlbuffer(xmlFile, "procName", arg.procName, 50); // 进程名。
    if (strlen(arg.procName) == 0)
    {
        logFile.write("procName is null.\n");
        return false;
    }
    return true;
}
/**
 * @brief        : 将获取到的list文件加载到nlistFiles容器中
 * @return        {bool} : 加载成功返回true,否则返回false
 **/
bool loadLocalFileList()
{
    nlistFiles.clear();
    cdir dir;
    if (dir.opendir(arg.localPath, arg.matchName) == false)
    {
        logFile.write("dir.opendir(%s) failed.\n", arg.localPath);
        return false;
    }

    string strFileName; // 存取读取的一行
    while (true)
    {
        if (dir.readdir() == false) // 读取一行失败
        {
            break;
        }

        nlistFiles.emplace_back(dir.m_filename, dir.m_mtime); // 加入容器
    }

    for (auto &file : nlistFiles)
    {
        logFile.write("fileName = %s,editTime=%s\n", file.fileName.c_str(), file.editTime.c_str());
    }

    return true;
}
/**
 * @brief        : 加载已经下载的文件名到容器一uploadedFilesName中
 * @return        {*}
 **/
bool loadUpoadedFiles()
{
    uploadedFiles.clear();
    cifile inFile;
    // 如果程序第一次运行,arg.uploadFileName是不存在的，并不是错误，所以也返回true。
    if (inFile.open(arg.uploadFileName) == false)
    {
        return true;
    }
    string stringBuffer;
    fileInfo_t fileInfo;
    while (inFile.readline(stringBuffer) != false)
    {
        fileInfo.clear();
        getxmlbuffer(stringBuffer, "fileName", fileInfo.fileName);
        getxmlbuffer(stringBuffer, "editTime", fileInfo.editTime);

        uploadedFiles[fileInfo.fileName] = fileInfo.editTime;
    }

    // for (const auto &[fileName, editTime] : uploadedFiles)
    // {
    //     logFile.write("fileName = %s , editTime = %s\n", fileName, editTime);
    // }
    return true;
}
/**
 * @brief        : 比较不同容器的文件信息更新skippedFiles和filesToUpload
 * @return        {void}
 **/
void cmpContainer()
{
    filesToUpload.clear();
    skippedFiles.clear();
    // 遍历nlistFiles
    for (const auto &nFile : nlistFiles)
    {
        auto it = uploadedFiles.find(nFile.fileName); // 在已下载的文件中查找nlistFiles中的文件,
        if (it == uploadedFiles.end())                // 未找到,需要上传
        {
            filesToUpload.push_back(nFile);
            continue;
        }
        // 找到了
        if (it->second == nFile.editTime) // 时间相同不需要下载
        {
            skippedFiles.push_back(nFile);
        }
        else // 时间不同需要下载
        {
            filesToUpload.push_back(nFile);
        }
    }
}
/**
 * @brief        : 把容器skippedFiles中的数据写入arg,uploadFileName文件,覆盖之前的
 * @return        {*}
 **/
void writeSkippedFiles()
{
    cofile outFile;

    if (outFile.open(arg.uploadFileName) == false)
    {
        logFile.write("outFile.open(%s) failed.", arg.uploadFileName);
    }

    for (const auto &skippedFile : skippedFiles)
    {
        outFile.writeline("<fileName>%s</fileName>\n<editTime>%s</editTime>", skippedFile.fileName.c_str(), skippedFile.editTime.c_str());
    }
    outFile.closeandrename();
}
/**
 * @brief        : 把下载成功的文件记录追加到arg.uploadFileName文件中。
 * @param         {fileInfo_t&} fileInfo: 文件信息
 * @return        {void}
 **/
void appendSkippedFiles(const fileInfo_t &fileInfo)
{
    cofile outFile;

    if (outFile.open(arg.uploadFileName, false, ios::app) == false)
    {
        logFile.write("outFile.open(%s) failed.\n", arg.uploadFileName);
        return;
    }
    outFile.writeline("<fileName>%s</fileName>\n<editTime>%s</editTime>", fileInfo.fileName.c_str(), fileInfo.editTime.c_str());
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
