# 此脚本用于停止所有的服务程序
# 停止调度程序
killall -9 procctl
# 停止其他的服务程序
killall crtsurfdata
# 让其服务程序有足够的时间退出
sleep 5
# 强制杀死
killall -9 crtsurfdata