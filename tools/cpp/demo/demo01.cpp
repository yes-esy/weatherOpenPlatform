/**
 * @FilePath     : /dataOpenPlatform/tools/cpp/server.cpp
 * @Description  :
 * @Author       : yes-esy 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : yes-esy 2900226123@qq.com
 * @LastEditTime : 2025-10-01 21:22:27
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "_public.h"
using namespace idc;

/**
 * 进程心跳信息结构体
 */
struct procInfo_t
{
    int pId = 0;            // 进程id
    char pName[51];         // 进程名称
    int timeout;            // 超时时间, 单位:秒
    time_t activeTime = 0;  // 最后一次心跳的时间
    procInfo_t() = default; // 有了自定义的构造函数,编译器将不提供默认构造函数,所以启用默认构造函数
    procInfo_t(const int in_pId, const string &in_pName, const int in_timeout, const time_t in_activeTime) : pId(in_pId), timeout(in_timeout), activeTime(in_activeTime)
    {
        strncpy(pName, in_pName.c_str(), 50);
    }
};

int shareMemId = -1;                // 共享内存Id
int shareMemPos = -1;               // 存放当前进程在数组下标的下标
procInfo_t *shareMemAddr = nullptr; // 指向共享内存地址空间
void EXIT(int sig);                 // 退出信号处理函数
int main()
{
    // 处理退出信号
    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);
    // 创建获取共享内存
    shareMemId = shmget((key_t)0x5095, 1000 * sizeof(procInfo_t), 06666 | IPC_CREAT); // 创建共享内存
    if (shareMemId == -1)                                                             // 创建失败退出
    {
        printf("创建/获取共享内存(%x)失败.\n", 0x5095);
        return -1;
    }

    // 将共享内存连接到当前进程的地址空间
    shareMemAddr = (procInfo_t *)shmat(shareMemId, 0, 0);
    // 把当前进程的信息填充到进程心跳信息结构体中
    procInfo_t procInfo(getpid(), "server", 30, time(0));
    // memset(&procInfo, 0, sizeof(procInfo_t));
    // procInfo.pId = getpid();
    // strncpy(procInfo.pName, "server", 50);
    // procInfo.timeout = 30;
    // procInfo.activeTime = time(0);
    csemp sem;             // 用于共享内存枷锁的信号量id
    if (!sem.init(0x5095)) // 初始化信号量失败
    {
        printf("获取创建信号量(%x)失败。\n", 0x5095);
        EXIT(-1);
    }
    sem.wait();
    // 进程id循环使用，如果有一个进程异常退出,没有清理自己的心跳信息
    // 他的进程信息将残留在共享内存中,不巧的是,如果当前进程重用了id
    // 所以,如果共享内存中已存在当前进程编号,一定是其他进程残留的信息,当前进程应该重用这个位置
    for (int i = 0; i < 1000; i++)
    {
        if (shareMemAddr[i].pId == procInfo.pId)
        {
            shareMemPos = i;
            printf("找到一个旧位置i=%d", i);
            break;
        }
    }
    if (shareMemPos < 0) // 不存在旧位置
    {
        // 在共享内存中寻找一个空位置，把当前进程的结构体保存到共享内存中。
        for (int i = 0; i < 1000; i++)
        {
            if (shareMemAddr[i].pId == 0) // pid为0表示是一个空位置
            {
                shareMemPos = -1;
                printf("找到一个新位置i=%d", i);
                break;
            }
        }
    }
    if (shareMemPos < 0) // 没有找到
    {
        sem.post(); // 共享空间内存用完解锁
        printf("共享内存空间已用完。\n");
        EXIT(-1);
    }
    // 把当前进程的结构体保存到共享内存中
    memcpy(&shareMemAddr[shareMemPos], &procInfo, sizeof(procInfo_t));
    sem.post(); // 成功找到位置解锁
    while (1)
    {
        printf("服务程序正在运行中...\n");
        sleep(25);
        shareMemAddr[shareMemPos].activeTime = time(0);
        sleep(25);
        shareMemAddr[shareMemPos].activeTime = time(0);
    }
    return 0;
}
/**
 * @brief        : 程序退出和信号处理函数
 * @param         {int} sig: 信号
 * @return        {void}
 **/
void EXIT(int sig)
{
    printf("sig=%d", sig);

    // 从共享内存删除当前进程的心跳信息
    if (shareMemPos != -1)
    {
        memset(&shareMemAddr[shareMemPos], 0, sizeof(procInfo_t));
    }
    // 把共享内存从当前进程分离
    if (shareMemAddr != 0)
    {
        shmdt(shareMemAddr);
    }
    exit(0);
}