SHELL = /bin/sh

#1、定义您自己的宏，比如您自己模块根目录
#2、定义您自己的可执行文件名称
SHARED_LIB_NAME=$(WORK_ROOT)/lib/libend_tas.so

##################################################################### #

#3、指定您必须生成的工程文件
#通过遍历目录(包括子目录)获取文件;最多遍历2层的子目录
END_TAS_DIR=$(WORK_ROOT)/src/vbh_server/endorser/plugin/end_tas
SOURCE = $(wildcard $(END_TAS_DIR)/*.cpp)
##################################################################### #

#5、指定您附加的搜索编译目录，不要指到其他模块内部
USER_CC_FLAG = -I$(WORK_ROOT)/src/vbh_server \
	-I$(WORK_ROOT)/src/vbh_server/endorser/plugin

######################################################################

#5、指定您附加的编译开关，通常为宏定义，例如：-DMMS_DEBUG  暂时只对下面3种系统的编译选项修改
# AIX 需要加 -qtls=local-dynamic  HP-UX 需要加 -mt  Linux 需要加  -fvisibility=hidden -fvisibility-inlines-hidden
USER_CC_FLAG += -DDECLARE_TRANSFORM_AGENT_EXPORT -fPIC -fvisibility=hidden -fvisibility-inlines-hidden
######################################################################

#7、指定您附加的库文件搜索目录，例如：-L./MYSUBDIR
USER_CC_LINK = -L$(WORK_ROOT)/lib

######################################################################

#8、指定您附加的库文件，例如：-lhello（假设您的动态库名称为libhello.so）
USER_CC_LINK += -ldsc -lvbh_comm -lvbh_server_comm

######################################################################

#9、包含统一文件，用来定义编译环境，不要更改
.PHONY: all
all: $(SHARED_LIB_NAME)
	@echo "[Complier $(SHARED_LIB_NAME) end time -- `date`]"

ACE=1
include $(WORK_ROOT)/mk_inc/rules.mak
