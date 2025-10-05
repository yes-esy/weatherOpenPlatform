# 此脚本用于启动所有的服务程序
# 启动守护模块
/project/dataOpenPlatform/tools/bin/procctl 10 /project/dataOpenPlatform/tools/bin/checkproc /tmp/log/checkproc.log

# 本程序用于生成气象站点观测的分钟数据，程序每分钟运行一次，由调度模块启动。
/project/dataOpenPlatform/tools/bin/procctl 60 /project/dataOpenPlatform/idc/bin/crtsurfdata /project/dataOpenPlatform/idc/ini/stcode.ini /tmp/idc/surfdata /log/idc/crtsurfdata.log csv,xml,json