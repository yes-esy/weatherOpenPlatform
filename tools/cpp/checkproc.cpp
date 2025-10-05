/**
 * @FilePath     : /dataOpenPlatform/tools/cpp/checkproc.cpp
 * @Description  :  进程守护模块,检查共享内存中进程的心跳,如果超时,则终止进程
 * @Author       : yes-esy 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : yes-esy 2900226123@qq.com
 * @LastEditTime : 2025-10-05 15:07:22
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "_public.h"
using namespace idc;
int main(int argc, char *argv[])
{
    // 程序的帮助。
    if (argc != 2)
    {
        printf("\n");
        printf("Using:./checkproc logfilename\n");

        printf("Example:/project/dataOpenPlatform/tools/bin/procctl 10 /project/dataOpenPlatform/tools/bin/checkproc /tmp/log/checkproc.log\n\n");

        printf("本程序用于检查后台服务程序是否超时，如果已超时，就终止它。\n");
        printf("注意：\n");
        printf("  1）本程序由procctl启动，运行周期建议为10秒。\n");
        printf("  2）为了避免被普通用户误杀，本程序应该用root用户启动。\n");
        printf("  3）如果要停止本程序，只能用killall -9 终止。\n\n\n");

        return 0;
    }
    // 忽略全部的信号和IO,不处程序的退出信号。
    closeioandsignal(true);
    // 打开日志文件。
    clogfile logFile; // 程序运行的日志。

    if (!logFile.open(argv[1])) // 打开失败
    {
        printf("logFile.open(%s) failed.\n", argv[1]);
        return -1;
    }
    // 创建/获取共享内存,键值为SHMKEYP,大小为MAXNUMP个st_procinfo结构体的大小。
    int shmId = shmget((key_t)SHMKEYP, MAXNUMP * sizeof(st_procinfo), 0666 | IPC_CREAT);
    if (shmId < 0)
    {
        logFile.write("创建/获取共享内存(%x)失败.\n", SHMKEYP);
        return -1;
    }
    // 将共享内存连接到当前进程的地址空间
    st_procinfo *shm = (st_procinfo *)shmat(shmId, 0, 0);

    // 遍历共享内存中全部的记录,如果进程已超时,终止它
    for (int i = 0; i < MAXNUMP; i++)
    {
        if (shm[i].pid == 0) // pid = 0 , 表示是服务程序的心跳记录
        {
            continue;
        }
        // logFile.write("i=%d,pid=%d,pname=%s,timeout=%d,atime=%d\n",
        //               i, shm[i].pid, shm[i].pname, shm[i].timeout, shm[i].atime);

        // 如果进程已经不存在,共享内存中是残留心跳信息
        // 向进程发送信号0,判断它是否还存在,如果不存在,从共享内存中删除该记录,continue;
        int iret = kill(shm[i].pid,0);
        if(iret == -1)
        {
            logFile.write("进程pid=%d(%s)已经不存在。\n",shm[i].pid,shm[i].pname);
            continue;
        }
        // 判断进程的心跳是否超时,如果超时,终止它
        time_t now = time(0);
        if(now - shm[i].atime < shm[i].timeout) // 未超时
        {
            continue;
        }

        st_procinfo tmp= shm[i];
        if(tmp.pid == 0)
        {
            continue;
        }
        // 已经超时
        // logFile.write("进程pid=%d(%s)已经超时.\n",tmp.pid,tmp.pname);

        // 发送信号15,终止已超时的进程
        kill(tmp.pid,15);
        for(int j = 0 ; j < 5 ; j ++)
        {
            sleep(1);
            iret = kill(tmp.pid,0); // 向进程发送信号0,判断它是否还存在
            if(iret == -1) // 进程已退出
            {
                break;
            }
        }
        if (iret == -1) // 正常退出
        {
            logFile.write("进程pid=%d(%s)已经正常结束。\n", tmp.pid, tmp.pname);
        }
        else
        {
            kill(tmp.pid,9);
            logFile.write("进程pid=%d(%s)已经强制终止。\n",tmp.pid,tmp.pname);

            // 从共享内存中删除已近超时进程的心跳记录
            memset(shm+i,0,sizeof(st_procinfo));
        }
    }
    // 把共享内存从当前进程中分离。
    shmdt(shm);

    return 0;
}