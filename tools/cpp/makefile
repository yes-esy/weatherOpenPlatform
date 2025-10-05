
# 开发框架头文件路径。
PUBINCL = -I../../public

# 开发框架cpp文件名，这里直接包含进来，没有采用链接库，是为了方便调试。
PUBCPP = ../../public/_public.cpp

# 编译选项
CFLAGS = -g
# 编译参数。
# CFLAGS = -O2


all:procctl checkproc

procctl:procctl.cpp
		g++ -g -o procctl procctl.cpp
		cp -f procctl ../bin/.

checkproc:checkproc.cpp
		g++ $(CFLAGS) -o checkproc checkproc.cpp $(PUBINCL) $(PUBCPP)
		cp -f checkproc ../bin/.
clean:
		rm -rf procctl checkproc
