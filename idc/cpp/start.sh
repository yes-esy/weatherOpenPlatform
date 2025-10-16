# 此脚本用于启动所有的服务程序
# 启动守护模块
/project/dataOpenPlatform/tools/bin/procctl 10 /project/dataOpenPlatform/tools/bin/checkproc /tmp/log/checkproc.log

# 本程序用于生成气象站点观测的分钟数据，程序每分钟运行一次，由调度模块启动。
/project/dataOpenPlatform/tools/bin/procctl 60 /project/dataOpenPlatform/idc/bin/crtsurfdata /project/dataOpenPlatform/idc/ini/stcode.ini /tmp/idc/surfdata /log/idc/crtsurfdata.log csv,xml,json

# 定期删除/tmp/idc/surfdata目录下的文件
/project/dataOpenPlatform/tools/bin/procctl 300 /project/dataOpenPlatform/tools/bin/deleteFiles /tmp/idc/surfdata "*" 0.02

# 压缩后台服务重新的备份日志
/project/dataOpenPlatform/tools/bin/procctl 300 /project/dataOpenPlatform/tools/bin/gzipFiles /log/idc "*.log.20" 0.02

# 从/tmp/idc/surfdata目录下载原始的气象观测数据文件，存放在/idcdata/surfdata目录。
/project/dataOpenPlatform/tools/bin/procctl 30 /project/dataOpenPlatform/tools/bin/ftpGetFiles /log/idc/ftpGetFiles_surfdata.log \
"<host>111.228.47.8:21</host>\
<mode>1</mode>\
<username>yes</username>\
<password>123456789yes</password>\
<localPath>/idcdata/surfdata</localPath>\
<remotePath>/tmp/idc/surfdata</remotePath>\
<matchName>SURF_ZH*.XML,SURF_ZH*.CSV</matchName>\
<listfilename>/idcdata/ftplist/ftpGetFiles_surfdata.list</listfilename>\
<pType>1</pType>\
<downloadedFileList>/idcdata/ftplist/ftpGetFiles_surfdata.xml</downloadedFileList>\
<checkmtime>true</checkmtime>\
<timeout>80</timeout>\
<pName>ftpGetFiles_surfdata</pName>"

# 清理/idcdata/surfdata目录中0.04天之前的文件。
/project/dataOpenPlatform/tools/bin/procctl 300 /project/dataOpenPlatform/tools/bin/deleteFiles /idcdata/surfdata "*" 0.04

# 把/tmp/idc/surfdata目录的原始气象观测数据文件上传到/tmp/ftpGetFiles目录。
# 注意，先创建好服务端的目录：mkdir /tmp/ftpputest 
/project/dataOpenPlatform/tools/bin/procctl 30 /project/dataOpenPlatform/tools/bin/ftpPutfiles /log/idc/ftpGetFiles_surfdata.log \
"<host>111.228.47.8:21</host>\
<mode>1</mode>\
<username>yes</username>\
<password>123456789yes</password>\
<localPath>/idcdata/surfdata</localPath>\
<remotePath>/tmp/idc/surfdata</remotePath>\
<matchName>SURF_ZH*.XML,SURF_ZH*.CSV</matchName>\
<pType>1</pType>\
<uploadedFileList>/idcdata/ftplist/ftpputfiles_surfdata.xml</uploadedFileList>\
<timeout>80</timeout>\
<pName>ftpPutTest_surfdata</pName>"

# 清理/tmp/ftpputest目录中0.04天之前的文件。
/project/dataOpenPlatform/tools/bin/procctl 300 /project/dataOpenPlatform/tools/bin/deleteFiles /tmp/ftpPutTest "*" 0.04

