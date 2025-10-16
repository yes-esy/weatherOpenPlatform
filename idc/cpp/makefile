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

# 编译参数。
CFLAGS = -g

all:crtsurfdata obtainCodeToDB

crtsurfdata:crtsurfdata.cpp
	g++ $(CFLAGS) -o crtsurfdata crtsurfdata.cpp $(PUBINCL) $(PUBCPP) -lm -lc
	cp -f crtsurfdata ../bin/.

obtainCodeToDB:obtainCodeToDB.cpp
	g++ $(CFLAGS) -o obtainCodeToDB obtainCodeToDB.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORALIB) $(ORALIBS) $(ORACPP)
	cp -f obtainCodeToDB ../bin/.


clean:
	rm -f crtsurfdata obtainCodeToDB