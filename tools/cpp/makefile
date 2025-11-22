
# 开发框架头文件路径。
PUBINCL = -I../../public

# 开发框架cpp文件名，这里直接包含进来，没有采用链接库，是为了方便调试。
PUBCPP = ../../public/_public.cpp

################################
# oracle头文件路径
ORAINCL = -I$(ORACLE_HOME)/rdbms/public -I/project/dataOpenPlatform/public/db/oracle

# oracle库文件路径
ORALIB =  -L$(ORACLE_HOME)/lib -L.

# oracle的oci库
ORALIBS = -lclntsh

# 开发框架oracle的cpp文件名，这里直接包含进来，没有采用链接库，是为了方便调试。
ORACPP = /project/dataOpenPlatform/public/db/oracle/_ooci.cpp
################################

# 编译选项
CFLAGS = -g
# 编译参数。
# CFLAGS = -O2


all:procctl checkproc deleteFiles gzipFiles ftpGetFiles ftpPutFiles dMiningOracle xmlToDB deleteTable migrateTable syncRef syncIncr
 
procctl:procctl.cpp
		g++ -g -o procctl procctl.cpp
		cp -f procctl ../bin/.

checkproc:checkproc.cpp
		g++ $(CFLAGS) -o checkproc checkproc.cpp $(PUBINCL) $(PUBCPP)
		cp -f checkproc ../bin/.

deleteFiles:deleteFiles.cpp
		g++ $(CFLAGS) -o deleteFiles deleteFiles.cpp $(PUBINCL) $(PUBCPP)
		cp -f deleteFiles ../bin/.

gzipFiles:gzipFiles.cpp
		g++ $(CFLAGS) -o gzipFiles gzipFiles.cpp $(PUBINCL) $(PUBCPP)
		cp -f gzipFiles ../bin/.

ftpGetFiles:ftpGetFiles.cpp
		g++ $(CFLAGS) -o ftpGetFiles ftpGetFiles.cpp $(PUBINCL) $(PUBCPP) /project/dataOpenPlatform/public/libftp.a /project/dataOpenPlatform/public/_ftp.cpp
		cp -f ftpGetFiles ../bin/.

ftpPutFiles:ftpPutFiles.cpp
		g++ $(CFLAGS) -o ftpPutFiles ftpPutFiles.cpp $(PUBINCL) $(PUBCPP) /project/dataOpenPlatform/public/libftp.a /project/dataOpenPlatform/public/_ftp.cpp
		cp -f ftpPutFiles ../bin/.
dMiningOracle:dMiningOracle.cpp
	g++ $(CFLAGS) -o dMiningOracle dMiningOracle.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL)  $(ORALIB) $(ORALIBS) $(ORACPP)
	cp -f dMiningOracle ../bin/.
xmlToDB:xmlToDB.cpp
	g++ $(CFLAGS) -o xmlToDB xmlToDB.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL)  $(ORALIB) $(ORALIBS) $(ORACPP) _tools.cpp
	cp -f xmlToDB ../bin/.
deleteTable:deleteTables.cpp
	g++ $(CFLAGS) -o deleteTable deleteTables.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL)  $(ORALIB) $(ORALIBS) $(ORACPP) 
	cp -f deleteTable ../bin/.
migrateTable:migrateTable.cpp
	g++ $(CFLAGS) -o migrateTable migrateTable.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL)  $(ORALIB) $(ORALIBS) $(ORACPP) _tools.cpp
	cp -f migrateTable ../bin/.
syncRef:syncRef.cpp
	g++ $(CFLAGS) -o syncRef syncRef.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL)  $(ORALIB) $(ORALIBS) $(ORACPP) _tools.cpp
	cp -f syncRef ../bin/.

syncIncr:syncIncr.cpp
	g++ $(CFLAGS) -o syncIncr syncIncr.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL)  $(ORALIB) $(ORALIBS) $(ORACPP) _tools.cpp
	cp -f syncIncr ../bin/.
clean:
		rm -rf procctl checkproc deleteFiles gzipFiles ftpGetFiles ftpPutFiles dMiningOracle xmlToDB deleteTable migrateTable syncRef syncIncr
