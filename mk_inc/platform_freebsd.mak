CC_FLAG += -I$(WORK_ROOT)/src -D_REENTRANT -w -D_GNU_SOURCE -D_FREEBSD_PLATFORM_ -DDSC_NO_DELAY  -fdelayed-template-parsing -pthread -m64 -pipe -std=c++11

ifeq ($(release),1)
  CC_FLAG += -w -O3
else
  #CC_FLAG += -g -fstack-protector -fno-inline -fno-default-inline  -fno-strict-aliasing -fsanitize=address -fsanitize=leak
  CC_FLAG += -g -fstack-protector -fno-inline -fno-default-inline
endif

ifeq ($(release),1)
  CC_LINK += -O3
else
  CC_LINK += -g -fstack-protector
endif

CC_LINK += -L$(WORK_ROOT)/lib -laxis2c -pthread -m64

include $(WORK_ROOT)/mk_inc/platform_common.mak

#define compiler
##################################################
CXX           = clang++
CCC           = CXX
CC            = clang
PIC           = -fpic
AR            = ar
ARFLAGS       = rsuv
SOLINK.cc = $(CXX) $(PIC) -shared -o
##################################################

#compile switch
##################################################