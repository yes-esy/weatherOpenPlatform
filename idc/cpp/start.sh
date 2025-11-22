# 此脚本用于启动所有的服务程序
# 启动守护模块
# /project/dataOpenPlatform/tools/bin/procctl 10 /project/dataOpenPlatform/tools/bin/checkproc /tmp/log/checkproc.log

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
<remotePath>/tmp/ftp-service/idc/surfdata</remotePath>\
<matchName>SURF_ZH*.XML,SURF_ZH*.CSV</matchName>\
<listFileName>/idcdata/ftplist/ftpGetFiles_surfdata.list</listFileName>\
<pType>1</pType>\
<downloadedFileList>/idcdata/ftplist/ftpGetFiles_surfdata.xml</downloadedFileList>\
<checkmtime>true</checkmtime>\
<timeout>80</timeout>\
<procName>ftpGetFiles</procName>\
<pName>ftpGetFiles_surfdata</pName>"

# 清理/idcdata/surfdata目录中0.04天之前的文件。
/project/dataOpenPlatform/tools/bin/procctl 300 /project/dataOpenPlatform/tools/bin/deleteFiles /idcdata/surfdata "*" 0.04/

# 把/tmp/idc/surfdata目录的原始气象观测数据文件上传到/tmp/ftpGetFiles目录。
# 注意，先创建好服务端的目录：mkdir /tmp/ftpputest 
/project/dataOpenPlatform/tools/bin/procctl 30 /project/dataOpenPlatform/tools/bin/ftpPutFiles /log/idc/ftpPutFiles_surfdata.log \
"<host>111.228.47.8:21</host>\
<mode>1</mode>\
<username>yes</username>\
<password>123456789yes</password>\
<localPath>/tmp/idc/surfdata</localPath>\
<remotePath>/tmp/ftp-service/ftp-upload</remotePath>\
<remotePathBak>/tmp/ftp-service/bak</remotePathBak>\n\
<matchName>SURF_ZH*.XML,SURF_ZH*.CSV</matchName>\
<pType>1</pType>\
<uploadedFileList>/idcdata/ftplist/ftpputfiles_surfdata.xml</uploadedFileList>\
<timeout>80</timeout>\
<procName>ftpPutTest_surfdata</procName>"

# 清理/tmp/ftpputest目录中0.04天之前的文件。
/project/dataOpenPlatform/tools/bin/procctl 300 /project/dataOpenPlatform/tools/bin/deleteFiles /tmp/ftpPutTest "*" 0.04



# 把站点参数数据入库到ZHOBTCODE表中，如果站点不存在则插入，站点已存在则更新。
/project/dataOpenPlatform/tools/bin/procctl 120 /project/dataOpenPlatform/idc/bin/obtainCodeToDB /project/idc/ini/stcode.ini "idc/idcpwd" "Simplified Chinese_China.AL32UTF8" /log/idc/obtcodetodb.log

# 把/idcdata/surfdata目录中的气象观测数据文件入库到T_ZHOBTMIND表中。
/project/dataOpenPlatform/tools/bin/procctl 10 /project/dataOpenPlatform/idc/bin/obtainMindToDB /idcdata/surfdata "idc/idcpwd" "Simplified Chinese_China.AL32UTF8" /log/idc/obtmindtodb.log

# 执行/project/dataOpenPlatform/idc/sql/deletetable.sql脚本，删除T_ZHOBTMIND表两小时之前的数据，如果启用了数据清理程序deletetable，就不必启用这行脚本了。
/project/dataOpenPlatform/tools/bin/procctl 120 /oracle/home/bin/sqlplus idc/idcpwd @/project/idc/sql/deletetable.sql

# 每隔1小时把T_ZHOBTCODE表中全部的数据抽取出来。
/project/dataOpenPlatform/tools/bin/procctl 3600 /project/dataOpenPlatform/tools/bin/dMiningOracle /log/idc/dminingoracle_ZHOBTCODE.log \
"<connectStr>idc/idcpwd</connectStr>\
<charset>Simplified Chinese_China.AL32UTF8</charset>\
<selectSQL>select site_id,city_name,province_name,latitude,longitude,height from T_ZHOBTCODE</selectSQL>\
<fieldStr>obtid,cityname,provname,lat,lon,height</fieldStr>\
<fieldLen>5,30,30,10,10,10</fieldLen>\
<bFileName>ZHOBTCODE</bFileName>\
<eFileName>toidc</eFileName>\
<outpath>/idcdata/dmindata</outpath>\
<timeout>30</timeout>\
<procName>dminingoracle_ZHOBTCODE</procName>"


# 每30秒从T_ZHOBTMIND表中增量抽取数据。
/project/dataOpenPlatform/tools/bin/procctl 30 /project/tools/bin/dminingoracle /log/idc/dminingoracle_ZHOBTMIND.log \
"<connectStr>idc/idcpwd</connectStr>\
<charset>Simplified Chinese_China.AL32UTF8</charset>\
<selectSQL>select site_id,to_char(visit_date,'yyyymmddhh24miss'),t,p,u,wd,wf,r,vis,key_id from T_ZHOBTMIND where  site_id like '5%%'</selectSQL>\
<fieldStr>site_id,visit_date,t,p,u,wd,wf,r,vis,key_id</fieldStr>\
<fieldLen>5,19,8,8,8,8,8,8,8,15</fieldLen>\
<bFileName>ZHOBTMIND</bFileName>\
<eFileName>togxpt</efilename\
<outpath>/idcdata/dmindata</outpath>\
<startTime></startTime>\
<incrField>key_id</incrField>\
<incrFileName>/idcdata/dmining/dminingoracle_ZHOBTMIND_togxpt.keyid</incrFileName>\
<timeout>30</timeout>\
<procName>dminingoracle_ZHOBTMIND_togxpt</procName>\
<maxCount>1000</maxCount>\
<connectStr1>scott/tiger@snorcl11g_128</connectStr1>"

# 清理/idcdata/dmindata目录中文件，防止把空间撑满。
/project/dataOpenPlatform/tools/bin/procctl 300 /project/tools/bin/deleteFiles /idcdata/dmindata "*" 0.02

# 把/idcdata/xmltodb/vip目录中的xml文件入库到T_ZHOBTCODE1和T_ZHOBTMIND1。
/project/dataOpenPlatform/tools/bin/procctl 10 /project/tools/bin/xmlToDB /log/idc/xmltodb_vip.log \
"<connectStr>idc/idcpwd</connectStr>\
<charset>Simplified Chinese_China.AL32UTF8</charset>\
<iniFileName>/project/dataOpenPlatform/idc/ini/xmltodb.xml</iniFileName>\
<xmlPath>/idcdata/xmltodb/vip</xmlPath>\
<xmlPathBak>/idcdata/xmltodb/vipbak</xmlPathBak>\
<xmlPathErr>/idcdata/xmltodb/viperr</xmlPathErr>\
<timeInterval>5</timeInterval>\
<timeout>50</timeout>\
<procName>xmltodb_vip</procName>"
# 注意，观测数据源源不断的入库到T_ZHOBTMIND1中，为了防止表空间被撑满，在/project/idc/sql/deletetable.sql中要配置清理T_ZHOBTMIND1表中历史数据的脚本。

# 清理/idcdata/xmltodb/vipbak和/idcdata/xmltodb/viperr目录中文件。
/project/dataOpenPlatform/tools/bin/procctl 300 /project/dataOpenPlatform/tools/bin/deleteFiles /idcdata/xmltodb/vipbak "*" 0.02
/project/dataOpenPlatform/tools/bin/procctl 300 /project/dataOpenPlatform/tools/bin/deleteFiles /idcdata/xmltodb/viperr  "*" 0.02

/project/dataOpenPlatform/tools/bin/procctl 3600 /project/dataOpenPlatform/tools/bin/deletetable /log/idc/deletetable_ZHOBTMIND1.log \
           "<connStr>idc/idcpwd</connStr>\
           <tableName>T_ZHOBTMIND1</tableName>\
           <keyCol>rowid</keyCol>\
           <where>where ddatetime<sysdate-0.03</where>\
           <maxCount>10</maxCount>\
           <startTime>22,23,00,01,02,03,04,05,06,13</startTime>\
           <timeout>120</timeout>\
           <procName>deletetable_ZHOBTMIND1</procName>\