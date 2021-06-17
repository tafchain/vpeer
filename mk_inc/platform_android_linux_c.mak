OUTPUT_FLAG = -o $@

CC_FLAG = -I$(WORK_ROOT)/src -Wpointer-arith -D_REENTRANT -D_GNU_SOURCE -w -pipe
ifeq ($(release),1)
CC_FLAG += -O3 -flto
else
CC_FLAG += -ggdb -fstack-protector -fno-inline -fno-default-inline
endif

ifeq ($(release),1)
CC_LINK += -O3 -flto
else
CC_LINK += -ggdb -fstack-protector
endif

#define compiler
##################################################
ifeq ($(ANDROID_ABI),x86)
CXX           = i686-linux-android-clang
CCC           = CXX
CC            = i686-linux-android-clang
AR            = i686-linux-android-ar
endif

ARFLAGS       = rsuv
PIC           = -fPIC
SOLINK.cc = $(CXX) $(PIC) -shared -o
##################################################

#compile switch
##################################################
