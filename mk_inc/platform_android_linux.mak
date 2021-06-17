CC_FLAG += -I$(WORK_ROOT)/src -D_REENTRANT -w -D_GNU_SOURCE -DUSING_EPOLL -D_ANDROID_PLATFORM_ -DACE_ANDROID_NDK_HAS_NDK_VERSION_H -pipe

ifeq ($(release),1)
  CC_FLAG += -w -O3
else
  CC_FLAG += -ggdb -fstack-protector -fno-inline -fno-default-inline
endif

ifeq ($(release),1)
  CC_LINK += -O3 -s
else
  CC_LINK += -ggdb -fstack-protector 
endif
CC_LINK += -L$(WORK_ROOT)/lib

include $(WORK_ROOT)/mk_inc/platform_common.mak

ifeq ($(ANDROID_ABI),armeabi-v7a)
  VBH_CROSS_COMPILE := arm-linux-androideabi-
endif

ifeq ($(ANDROID_ABI),arm64-v8a)
  VBH_CROSS_COMPILE := aarch64-linux-android-
endif

ifeq ($(ANDROID_ABI),x86)
  VBH_CROSS_COMPILE := i686-linux-android-
endif

ifeq ($(ANDROID_ABI),x86_64)
  VBH_CROSS_COMPILE := x86_64-linux-android-
endif

#define compiler
##################################################
CXX           = $(VBH_CROSS_COMPILE)clang++
CCC           = CXX
CC            = $(VBH_CROSS_COMPILE)clang
AR            = $(VBH_CROSS_COMPILE)ar

ARFLAGS       = rsuv
PIC           = -fPIC
SOLINK.cc = $(CXX) $(PIC) -shared -o
##################################################

#compile switch
##################################################
