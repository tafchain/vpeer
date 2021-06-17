SHELL = /bin/sh

##################################################################### #
#1、定义您自己的宏，比方您自己模块根目录
VBH_END_APP_DIR=$(WORK_ROOT)/src/vbh_server/endorser/end_app

#2、定义您自己的可执行文件名称
PROGRAM_NAME=$(WORK_ROOT)/bin/end_app

##################################################################### #
#3、指定您必须生成的工程文件

SOURCE  = $(wildcard $(VBH_END_APP_DIR)/*.cpp)
ifeq ($(dsc_test),1)
  VBH_END_CES_DIR=$(WORK_ROOT)/src/vbh_server/endorser/plugin/end_ces
  SOURCE += $(wildcard $(VBH_END_CES_DIR)/*.cpp)
endif

##################################################################### #

#4、指定您附加的搜索编译目录，不要指到其他模块内部
USER_CC_FLAG = -I$(WORK_ROOT)/src/vbh_server
ifeq ($(dsc_test),1)
  USER_CC_FLAG += -fPIC -fvisibility=hidden -fvisibility-inlines-hidden
  USER_CC_FLAG += -I$(WORK_ROOT)/src/vbh_server/endorser/plugin \
		-I$(WORK_ROOT)/src/vbh_server/cc
endif
#6、指定您附加的库文件搜索目录，例如：-L./MYSUBDIR
#USER_CC_LINK = -L$(WORK_ROOT)/lib

#7、指定您附加的库文件，例如：-lhello（假设您的动态库名称为libhello.so）
USER_CC_LINK += -ldsc -lvbh_comm -lvbh_server_comm
ifeq ($(dsc_test),1)
  USER_CC_LINK += -lend_tas -lcc_comm -lend_comm
endif
#####################################################################
#
#8、包含统一文件，用来定义编译环境，不要更改
.PHONY: all
all: $(PROGRAM_NAME)
	@echo "[Complier $(PROGRAM_NAME) end time -- `date`]"

ACE=1
OPENSSL=1
include $(WORK_ROOT)/mk_inc/rules.mak

