# 此脚本用于停止所有的服务程序
# 停止调度程序
killall -9 procctl
# 停止其他的服务程序
killall crtsurfdata deleteFiles gzipFiles ftpGetFiles ftpPutFiles 
# 让其服务程序有足够的时间退出
sleep 5
# 强制杀死
# 不管服务程序有没有退出，都强制杀死。
killall -9 crtsurfdata deleteFiles gzipFiles ftpGetFiles ftpPutFiles
killall -9 obtainMindToDB
killall -9 dMiningOracle xmlToDB 